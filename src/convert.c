#include "convert.h"

#include "miniaudio.h"
#include "silk_encoder.h"

#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

static int16_t float_to_pcm16(float sample) {
    if (sample > 1.0f) sample = 1.0f;
    if (sample < -1.0f) sample = -1.0f;
    const float scaled = sample * 32767.0f;
    return (int16_t)(scaled >= 0.0f ? scaled + 0.5f : scaled - 0.5f);
}

ConvertStatus convert_file(const wchar_t *input_path, const wchar_t *output_path) {
    if (!input_path || !output_path || !input_path[0] || !output_path[0]) {
        return CONVERT_INVALID_ARGUMENTS;
    }

    ma_decoder_config config = ma_decoder_config_init(ma_format_f32, 1, 24000);
    ma_decoder decoder;
    if (ma_decoder_init_file_w(input_path, &config, &decoder) != MA_SUCCESS) {
        return CONVERT_INPUT_OPEN_FAILED;
    }

    FILE *output = _wfopen(output_path, L"wb");
    if (!output) {
        ma_decoder_uninit(&decoder);
        return CONVERT_OUTPUT_FAILED;
    }

    SilkEncoder *encoder = NULL;
    ConvertStatus status = CONVERT_OK;
    if (silk_encoder_open(&encoder, output) != 0) {
        status = CONVERT_ENCODE_FAILED;
        goto cleanup;
    }

    float samples[2400];
    int16_t pcm[2400];
    for (;;) {
        ma_uint64 frames_read = 0;
        const ma_result result = ma_decoder_read_pcm_frames(&decoder, samples, 2400, &frames_read);
        if (result != MA_SUCCESS && !(result == MA_AT_END && frames_read == 0)) {
            status = CONVERT_DECODE_FAILED;
            break;
        }
        for (ma_uint64 i = 0; i < frames_read; ++i) {
            pcm[i] = float_to_pcm16(samples[i]);
        }
        if (frames_read > 0 && silk_encoder_encode(encoder, pcm, (int)frames_read) != 0) {
            status = CONVERT_ENCODE_FAILED;
            break;
        }
        if (frames_read < 2400) {
            break;
        }
    }
    if (status == CONVERT_OK && silk_encoder_flush(encoder) != 0) {
        status = CONVERT_ENCODE_FAILED;
    }

cleanup:
    silk_encoder_close(encoder);
    ma_decoder_uninit(&decoder);
    if (fclose(output) != 0 && status == CONVERT_OK) {
        status = CONVERT_OUTPUT_FAILED;
    }
    if (status != CONVERT_OK) {
        _wremove(output_path);
    }
    return status;
}
