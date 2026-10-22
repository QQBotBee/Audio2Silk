#ifndef AUDIO2SILK_SILK_ENCODER_H
#define AUDIO2SILK_SILK_ENCODER_H

#include <stdint.h>
#include <stdio.h>

typedef struct SilkEncoder SilkEncoder;

int silk_encoder_open(SilkEncoder **encoder, FILE *output);
int silk_encoder_encode(SilkEncoder *encoder, const int16_t *samples, int sample_count);
int silk_encoder_flush(SilkEncoder *encoder);
void silk_encoder_close(SilkEncoder *encoder);

#endif
