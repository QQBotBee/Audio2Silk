#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

#include "silk_encoder.h"

int main(void) {
    FILE *file = tmpfile();
    assert(file != NULL);

    SilkEncoder *encoder = NULL;
    assert(silk_encoder_open(&encoder, file) == 0);

    int16_t samples[480];
    for (int i = 0; i < 480; ++i) {
        samples[i] = (int16_t)((i % 32) * 500);
    }
    assert(silk_encoder_encode(encoder, samples, 480) == 0);
    assert(silk_encoder_flush(encoder) == 0);
    silk_encoder_close(encoder);

    fflush(file);
    fseek(file, 0, SEEK_END);
    long size = ftell(file);
    assert(size > 11);
    rewind(file);

    unsigned char header[11] = {0};
    assert(fread(header, 1, sizeof(header), file) == sizeof(header));
    assert(header[0] == 0x02);
    assert(memcmp(header + 1, "#!SILK_V3", 9) == 0);
    fclose(file);
    return 0;
}
