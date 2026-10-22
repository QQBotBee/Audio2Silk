#include "silk_decoder.h"

#include "SKP_Silk_SDK_API.h"

#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <io.h>
#include <windows.h>

#define SILK_SAMPLE_RATE 24000
#define SILK_FRAME_SAMPLES (SILK_SAMPLE_RATE / 50)
#define SILK_MAX_PACKET_BYTES 1024

static int is_same_file(FILE *input, const wchar_t *output_path) {
    HANDLE output = CreateFileW(output_path, FILE_READ_ATTRIBUTES,
                               FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE,
                               NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
    if (output == INVALID_HANDLE_VALUE) {
        return 0;
    }
    BY_HANDLE_FILE_INFORMATION input_info, output_info;
    const HANDLE input_handle = (HANDLE)_get_osfhandle(_fileno(input));
    const int same = GetFileInformationByHandle(input_handle, &input_info) &&
                     GetFileInformationByHandle(output, &output_info) &&
                     input_info.dwVolumeSerialNumber == output_info.dwVolumeSerialNumber &&
                     input_info.nFileIndexHigh == output_info.nFileIndexHigh &&
                     input_info.nFileIndexLow == output_info.nFileIndexLow;
    CloseHandle(output);
    return same;
}

static int write_u16(FILE *file, uint16_t value) {
    const unsigned char bytes[2] = {(unsigned char)value, (unsigned char)(value >> 8)};
    return fwrite(bytes, 1, sizeof(bytes), file) == sizeof(bytes) ? 0 : -1;
}

static int write_u32(FILE *file, uint32_t value) {
    const unsigned char bytes[4] = {
        (unsigned char)value, (unsigned char)(value >> 8),
        (unsigned char)(value >> 16), (unsigned char)(value >> 24)
    };
    return fwrite(bytes, 1, sizeof(bytes), file) == sizeof(bytes) ? 0 : -1;
}

static int write_wav_header(FILE *file, uint32_t data_size) {
    return fwrite("RIFF", 1, 4, file) == 4 && write_u32(file, data_size + 36) == 0 &&
           fwrite("WAVEfmt ", 1, 8, file) == 8 && write_u32(file, 16) == 0 &&
           write_u16(file, 1) == 0 && write_u16(file, 1) == 0 &&
           write_u32(file, SILK_SAMPLE_RATE) == 0 &&
           write_u32(file, SILK_SAMPLE_RATE * 2) == 0 &&
           write_u16(file, 2) == 0 && write_u16(file, 16) == 0 &&
           fwrite("data", 1, 4, file) == 4 && write_u32(file, data_size) == 0 ? 0 : -1;
}

static int read_silk_header(FILE *input) {
    unsigned char header[9];
    int first = fgetc(input);
    if (first == 0x02) {
        if (fread(header, 1, sizeof(header), input) != sizeof(header)) {
            return -1;
        }
    } else if (first == '#') {
        header[0] = (unsigned char)first;
        if (fread(header + 1, 1, sizeof(header) - 1, input) != sizeof(header) - 1) {
            return -1;
        }
    } else {
        return -1;
    }
    return memcmp(header, "#!SILK_V3", sizeof(header)) == 0 ? 0 : -1;
}

static ConvertStatus decode_packets(FILE *input, FILE *output, void *decoder_state, uint64_t *data_bytes) {
    SKP_SILK_SDK_DecControlStruct control;
    memset(&control, 0, sizeof(control));
    control.API_sampleRate = SILK_SAMPLE_RATE;

    uint64_t total_bytes = 0;
    int has_packets = 0;
    unsigned char size_bytes[2];
    unsigned char payload[SILK_MAX_PACKET_BYTES];
    for (;;) {
        const size_t size_read = fread(size_bytes, 1, sizeof(size_bytes), input);
        if (size_read == 0 && feof(input)) {
            break;
        }
        if (size_read != sizeof(size_bytes)) {
            return CONVERT_DECODE_FAILED;
        }
        const unsigned int packet_size = (unsigned int)size_bytes[0] |
                                         ((unsigned int)size_bytes[1] << 8);
        if (packet_size == 0xFFFF) {
            if (fgetc(input) != EOF || ferror(input)) {
                return CONVERT_DECODE_FAILED;
            }
            break;
        }
        if (packet_size == 0 || packet_size > sizeof(payload) ||
            fread(payload, 1, packet_size, input) != packet_size) {
            return CONVERT_DECODE_FAILED;
        }

        int frames = 0;
        do {
            int16_t samples[SILK_FRAME_SAMPLES];
            SKP_int16 sample_count = 0;
            const SKP_int result = SKP_Silk_SDK_Decode(
                decoder_state, &control, 0, payload, (SKP_int)packet_size,
                samples, &sample_count);
            if (result != 0 || sample_count <= 0 || sample_count > SILK_FRAME_SAMPLES ||
                ++frames > SILK_MAX_FRAMES_PER_PACKET) {
                return CONVERT_DECODE_FAILED;
            }

            unsigned char pcm_bytes[SILK_FRAME_SAMPLES * 2];
            for (SKP_int16 i = 0; i < sample_count; ++i) {
                const uint16_t sample = (uint16_t)samples[i];
                pcm_bytes[i * 2] = (unsigned char)sample;
                pcm_bytes[i * 2 + 1] = (unsigned char)(sample >> 8);
            }
            const size_t frame_bytes = (size_t)sample_count * 2;
            if (total_bytes > UINT32_MAX - 36 - frame_bytes ||
                fwrite(pcm_bytes, 1, frame_bytes, output) != frame_bytes) {
                return CONVERT_OUTPUT_FAILED;
            }
            total_bytes += frame_bytes;
        } while (control.moreInternalDecoderFrames);
        has_packets = 1;
    }

    if (!has_packets || ferror(input)) {
        return CONVERT_DECODE_FAILED;
    }
    *data_bytes = total_bytes;
    return CONVERT_OK;
}

ConvertStatus decode_file(const wchar_t *input_path, const wchar_t *output_path) {
    if (!input_path || !output_path || !input_path[0] || !output_path[0]) {
        return CONVERT_INVALID_ARGUMENTS;
    }

    FILE *input = _wfopen(input_path, L"rb");
    if (!input) {
        return CONVERT_INPUT_OPEN_FAILED;
    }
    if (is_same_file(input, output_path)) {
        fclose(input);
        return CONVERT_INVALID_ARGUMENTS;
    }
    if (read_silk_header(input) != 0) {
        fclose(input);
        return CONVERT_DECODE_FAILED;
    }

    SKP_int32 state_size = 0;
    if (SKP_Silk_SDK_Get_Decoder_Size(&state_size) != 0 || state_size <= 0) {
        fclose(input);
        return CONVERT_DECODE_FAILED;
    }
    void *decoder_state = calloc(1, (size_t)state_size);
    if (!decoder_state) {
        fclose(input);
        return CONVERT_DECODE_FAILED;
    }
    if (SKP_Silk_SDK_InitDecoder(decoder_state) != 0) {
        free(decoder_state);
        fclose(input);
        return CONVERT_DECODE_FAILED;
    }

    const size_t output_length = wcslen(output_path);
    wchar_t *temp_path = (wchar_t *)malloc((output_length + 32) * sizeof(wchar_t));
    if (!temp_path) {
        free(decoder_state);
        fclose(input);
        return CONVERT_OUTPUT_FAILED;
    }
    _snwprintf(temp_path, output_length + 32, L"%ls.tmp.%lu", output_path,
               (unsigned long)GetCurrentProcessId());
    if (_waccess(temp_path, 0) == 0) {
        free(temp_path);
        free(decoder_state);
        fclose(input);
        return CONVERT_OUTPUT_FAILED;
    }

    FILE *output = _wfopen(temp_path, L"wb");
    if (!output) {
        free(temp_path);
        free(decoder_state);
        fclose(input);
        return CONVERT_OUTPUT_FAILED;
    }

    ConvertStatus status = CONVERT_OK;
    uint64_t data_bytes = 0;
    if (write_wav_header(output, 0) != 0) {
        status = CONVERT_OUTPUT_FAILED;
    } else {
        status = decode_packets(input, output, decoder_state, &data_bytes);
        if (status == CONVERT_OK && (fseek(output, 0, SEEK_SET) != 0 ||
            write_wav_header(output, (uint32_t)data_bytes) != 0)) {
            status = CONVERT_OUTPUT_FAILED;
        }
    }

    if (fclose(output) != 0 && status == CONVERT_OK) {
        status = CONVERT_OUTPUT_FAILED;
    }
    free(decoder_state);
    fclose(input);
    if (status != CONVERT_OK) {
        _wremove(temp_path);
    } else if (!MoveFileExW(temp_path, output_path, MOVEFILE_REPLACE_EXISTING | MOVEFILE_WRITE_THROUGH)) {
        _wremove(temp_path);
        status = CONVERT_OUTPUT_FAILED;
    }
    free(temp_path);
    return status;
}
