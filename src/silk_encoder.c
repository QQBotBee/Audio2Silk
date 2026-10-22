#include "silk_encoder.h"

#include "SKP_Silk_SDK_API.h"

#include <stdlib.h>
#include <string.h>

#define SILK_SAMPLE_RATE 24000
#define SILK_FRAME_SAMPLES 480
#define SILK_MAX_BYTES_PER_FRAME 250

struct SilkEncoder {
    FILE *output;
    void *state;
    SKP_SILK_SDK_EncControlStruct control;
    int16_t pending[SILK_FRAME_SAMPLES];
    int pending_count;
};

static int write_packet(SilkEncoder *encoder, const int16_t *samples) {
    SKP_uint8 payload[SILK_MAX_BYTES_PER_FRAME];
    SKP_int16 payload_size = (SKP_int16)sizeof(payload);
    const SKP_int result = SKP_Silk_SDK_Encode(
        encoder->state, &encoder->control, samples, SILK_FRAME_SAMPLES,
        payload, &payload_size);
    if (result != 0 || payload_size < 0) {
        return -1;
    }
    const uint16_t little_endian_size = (uint16_t)payload_size;
    if (fwrite(&little_endian_size, sizeof(little_endian_size), 1, encoder->output) != 1 ||
        fwrite(payload, 1, (size_t)payload_size, encoder->output) != (size_t)payload_size) {
        return -1;
    }
    return 0;
}

int silk_encoder_open(SilkEncoder **encoder_out, FILE *output) {
    if (!encoder_out || !output) {
        return -1;
    }

    SilkEncoder *encoder = (SilkEncoder *)calloc(1, sizeof(*encoder));
    if (!encoder) {
        return -1;
    }
    SKP_int32 state_size = 0;
    if (SKP_Silk_SDK_Get_Encoder_Size(&state_size) != 0 || state_size <= 0) {
        free(encoder);
        return -1;
    }
    encoder->state = calloc(1, (size_t)state_size);
    if (!encoder->state) {
        free(encoder);
        return -1;
    }
    SKP_SILK_SDK_EncControlStruct status;
    if (SKP_Silk_SDK_InitEncoder(encoder->state, &status) != 0) {
        free(encoder->state);
        free(encoder);
        return -1;
    }
    encoder->output = output;
    encoder->control.API_sampleRate = SILK_SAMPLE_RATE;
    encoder->control.maxInternalSampleRate = SILK_SAMPLE_RATE;
    encoder->control.packetSize = SILK_FRAME_SAMPLES;
    encoder->control.bitRate = 24000;
    encoder->control.packetLossPercentage = 0;
    encoder->control.complexity = 1;
    encoder->control.useInBandFEC = 0;
    encoder->control.useDTX = 0;

    static const unsigned char header[] = {0x02, '#', '!', 'S', 'I', 'L', 'K', '_', 'V', '3'};
    if (fwrite(header, 1, sizeof(header), output) != sizeof(header)) {
        free(encoder->state);
        free(encoder);
        return -1;
    }
    *encoder_out = encoder;
    return 0;
}

int silk_encoder_encode(SilkEncoder *encoder, const int16_t *samples, int sample_count) {
    if (!encoder || (!samples && sample_count > 0) || sample_count < 0) {
        return -1;
    }
    while (sample_count > 0) {
        const int to_copy = SILK_FRAME_SAMPLES - encoder->pending_count < sample_count
                                ? SILK_FRAME_SAMPLES - encoder->pending_count
                                : sample_count;
        memcpy(encoder->pending + encoder->pending_count, samples, (size_t)to_copy * sizeof(int16_t));
        encoder->pending_count += to_copy;
        samples += to_copy;
        sample_count -= to_copy;
        if (encoder->pending_count == SILK_FRAME_SAMPLES) {
            if (write_packet(encoder, encoder->pending) != 0) {
                return -1;
            }
            encoder->pending_count = 0;
        }
    }
    return 0;
}

int silk_encoder_flush(SilkEncoder *encoder) {
    if (!encoder) {
        return -1;
    }
    if (encoder->pending_count > 0) {
        memset(encoder->pending + encoder->pending_count, 0,
               (size_t)(SILK_FRAME_SAMPLES - encoder->pending_count) * sizeof(int16_t));
        if (write_packet(encoder, encoder->pending) != 0) {
            return -1;
        }
        encoder->pending_count = 0;
    }
    return fflush(encoder->output) == 0 ? 0 : -1;
}

void silk_encoder_close(SilkEncoder *encoder) {
    if (!encoder) {
        return;
    }
    free(encoder->state);
    free(encoder);
}
