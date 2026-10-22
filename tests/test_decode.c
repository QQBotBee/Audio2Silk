#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <io.h>

#include "convert.h"
#include "silk_encoder.h"
#include "silk_decoder.h"
#include "SKP_Silk_SDK_API.h"
#include <stdlib.h>

static void write_silk_fixture(const wchar_t *path) {
    FILE *file = _wfopen(path, L"wb");
    assert(file != NULL);
    SilkEncoder *encoder = NULL;
    assert(silk_encoder_open(&encoder, file) == 0);
    int16_t samples[960];
    for (int i = 0; i < 960; ++i) {
        samples[i] = (int16_t)((i % 80) * 300 - 12000);
    }
    assert(silk_encoder_encode(encoder, samples, 960) == 0);
    assert(silk_encoder_flush(encoder) == 0);
    silk_encoder_close(encoder);
    assert(fclose(file) == 0);
}

static void test_decodes_silk_to_pcm_wav(void) {
    const wchar_t *input = L"decode-test-input.silk";
    const wchar_t *output = L"decode-test-output.wav";
    write_silk_fixture(input);

    assert(decode_file(input, output) == CONVERT_OK);
    FILE *file = _wfopen(output, L"rb");
    assert(file != NULL);
    unsigned char header[44];
    assert(fread(header, 1, sizeof(header), file) == sizeof(header));
    assert(memcmp(header, "RIFF", 4) == 0);
    assert(memcmp(header + 8, "WAVEfmt ", 8) == 0);
    assert(header[20] == 1 && header[21] == 0);
    assert(header[22] == 1 && header[23] == 0);
    assert(header[24] == 0xC0 && header[25] == 0x5D && header[26] == 0 && header[27] == 0);
    assert(header[34] == 16 && header[35] == 0);
    const uint32_t data_size = (uint32_t)header[40] | ((uint32_t)header[41] << 8) |
                              ((uint32_t)header[42] << 16) | ((uint32_t)header[43] << 24);
    assert(data_size == 960u * sizeof(int16_t));
    assert(fseek(file, 0, SEEK_END) == 0);
    assert(ftell(file) == (long)(44 + data_size));
    fclose(file);
    _wremove(input);
    _wremove(output);
}

static void test_removes_partial_wav_for_truncated_packet(void) {
    const wchar_t *input = L"decode-test-truncated.silk";
    const wchar_t *output = L"decode-test-truncated.wav";
    FILE *file = _wfopen(input, L"wb");
    assert(file != NULL);
    static const unsigned char truncated[] = {
        0x02, '#', '!', 'S', 'I', 'L', 'K', '_', 'V', '3', 20, 0, 0x01, 0x02
    };
    assert(fwrite(truncated, 1, sizeof(truncated), file) == sizeof(truncated));
    fclose(file);

    assert(decode_file(input, output) == CONVERT_DECODE_FAILED);
    assert(_waccess(output, 0) != 0);
    _wremove(input);
}

static void test_standard_header_multiframe_packet_and_end_marker(void) {
    const wchar_t *input = L"decode-test-standard.silk";
    const wchar_t *output = L"decode-test-standard.wav";
    SKP_int32 size = 0;
    assert(SKP_Silk_SDK_Get_Encoder_Size(&size) == 0);
    void *state = calloc(1, (size_t)size);
    assert(state != NULL);
    SKP_SILK_SDK_EncControlStruct control;
    assert(SKP_Silk_SDK_InitEncoder(state, &control) == 0);
    control.API_sampleRate = 16000;
    control.maxInternalSampleRate = 16000;
    control.packetSize = 1600;
    control.bitRate = 24000;
    control.packetLossPercentage = 0;
    control.complexity = 1;
    control.useInBandFEC = 0;
    control.useDTX = 0;
    int16_t samples[1600];
    for (int i = 0; i < 1600; ++i) samples[i] = (int16_t)((i % 64) * 300 - 9600);
    unsigned char payload[1024];
    SKP_int16 payload_size = sizeof(payload);
    assert(SKP_Silk_SDK_Encode(state, &control, samples, 1600, payload, &payload_size) == 0);
    assert(payload_size > 0);
    free(state);

    FILE *file = _wfopen(input, L"wb");
    assert(file != NULL);
    assert(fwrite("#!SILK_V3", 1, 9, file) == 9);
    const unsigned char length[2] = {(unsigned char)payload_size, (unsigned char)(payload_size >> 8)};
    assert(fwrite(length, 1, 2, file) == 2);
    assert(fwrite(payload, 1, (size_t)payload_size, file) == (size_t)payload_size);
    assert(fwrite("\xFF\xFF", 1, 2, file) == 2);
    fclose(file);

    assert(decode_file(input, output) == CONVERT_OK);
    file = _wfopen(output, L"rb");
    assert(file != NULL);
    assert(fseek(file, 0, SEEK_END) == 0);
    assert(ftell(file) == 44 + 2400 * 2);
    fclose(file);
    _wremove(input);
    _wremove(output);
}

static void test_rejects_malformed_streams(void) {
    const wchar_t *input = L"decode-test-invalid.silk";
    const wchar_t *output = L"decode-test-invalid.wav";
    static const unsigned char fixtures[][16] = {
        {0x02, '#', '!', 'S', 'I', 'L', 'K', '_', 'V', '2'},
        {0x02, '#', '!', 'S', 'I', 'L', 'K', '_', 'V', '3'},
        {0x02, '#', '!', 'S', 'I', 'L', 'K', '_', 'V', '3', 1},
        {0x02, '#', '!', 'S', 'I', 'L', 'K', '_', 'V', '3', 0, 0},
        {0x02, '#', '!', 'S', 'I', 'L', 'K', '_', 'V', '3', 1, 4},
        {0x02, '#', '!', 'S', 'I', 'L', 'K', '_', 'V', '3', 4, 0, 0xFF, 0xFF, 0xFF, 0xFF}
    };
    const size_t sizes[] = {10, 10, 11, 12, 12, 16};
    for (size_t i = 0; i < sizeof(sizes) / sizeof(sizes[0]); ++i) {
        FILE *file = _wfopen(input, L"wb");
        assert(file != NULL);
        assert(fwrite(fixtures[i], 1, sizes[i], file) == sizes[i]);
        fclose(file);
        assert(decode_file(input, output) == CONVERT_DECODE_FAILED);
        assert(_waccess(output, 0) != 0);
    }
    _wremove(input);
}

static void test_same_file_is_preserved(void) {
    const wchar_t *input = L"decode-test-same.silk";
    write_silk_fixture(input);
    assert(decode_file(input, input) == CONVERT_INVALID_ARGUMENTS);
    FILE *file = _wfopen(input, L"rb");
    assert(file != NULL);
    assert(fgetc(file) == 0x02);
    fclose(file);
    _wremove(input);
}

static void test_invalid_header_preserves_existing_output(void) {
    const wchar_t *input = L"decode-test-header.silk";
    const wchar_t *output = L"decode-test-existing.wav";
    FILE *file = _wfopen(input, L"wb");
    assert(file != NULL);
    assert(fwrite("invalid", 1, 7, file) == 7);
    fclose(file);
    file = _wfopen(output, L"wb");
    assert(file != NULL);
    assert(fwrite("keep", 1, 4, file) == 4);
    fclose(file);
    assert(decode_file(input, output) == CONVERT_DECODE_FAILED);
    file = _wfopen(output, L"rb");
    assert(file != NULL);
    char content[4];
    assert(fread(content, 1, 4, file) == 4 && memcmp(content, "keep", 4) == 0);
    fclose(file);
    _wremove(input);
    _wremove(output);
}

static void test_corrupt_packet_preserves_existing_output(void) {
    const wchar_t *input = L"decode-test-corrupt.silk";
    const wchar_t *output = L"decode-test-existing-corrupt.wav";
    FILE *file = _wfopen(input, L"wb");
    assert(file != NULL);
    static const unsigned char bytes[] = {
        0x02, '#', '!', 'S', 'I', 'L', 'K', '_', 'V', '3', 20, 0, 0x01, 0x02
    };
    assert(fwrite(bytes, 1, sizeof(bytes), file) == sizeof(bytes));
    fclose(file);
    file = _wfopen(output, L"wb");
    assert(file != NULL);
    assert(fwrite("keep-corrupt", 1, 12, file) == 12);
    fclose(file);
    assert(decode_file(input, output) == CONVERT_DECODE_FAILED);
    file = _wfopen(output, L"rb");
    assert(file != NULL);
    char content[12];
    assert(fread(content, 1, sizeof(content), file) == sizeof(content));
    assert(memcmp(content, "keep-corrupt", sizeof(content)) == 0);
    fclose(file);
    _wremove(input);
    _wremove(output);
}

int main(void) {
    test_decodes_silk_to_pcm_wav();
    test_removes_partial_wav_for_truncated_packet();
    test_standard_header_multiframe_packet_and_end_marker();
    test_rejects_malformed_streams();
    test_same_file_is_preserved();
    test_invalid_header_preserves_existing_output();
    test_corrupt_packet_preserves_existing_output();
    return 0;
}
