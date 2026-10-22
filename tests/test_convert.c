#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <io.h>

#include "convert.h"

static void write_test_wav(const wchar_t *path) {
    FILE *file = _wfopen(path, L"wb");
    assert(file != NULL);
    const uint32_t sample_rate = 24000;
    const uint16_t channels = 1;
    const uint16_t bits = 16;
    const uint32_t frames = 24000;
    const uint32_t data_size = frames * sizeof(int16_t);
    const uint32_t list_size = 26;
    const uint32_t riff_size = 36 + data_size + 8 + list_size;
    fwrite("RIFF", 1, 4, file); fwrite(&riff_size, 4, 1, file); fwrite("WAVE", 1, 4, file);
    fwrite("fmt ", 1, 4, file); uint32_t fmt_size = 16; fwrite(&fmt_size, 4, 1, file);
    uint16_t format = 1; fwrite(&format, 2, 1, file); fwrite(&channels, 2, 1, file);
    fwrite(&sample_rate, 4, 1, file); uint32_t byte_rate = sample_rate * channels * bits / 8; fwrite(&byte_rate, 4, 1, file);
    uint16_t block_align = channels * bits / 8; fwrite(&block_align, 2, 1, file); fwrite(&bits, 2, 1, file);
    fwrite("LIST", 1, 4, file); fwrite(&list_size, 4, 1, file);
    fwrite("INFOISFT", 1, 8, file); uint32_t software_size = 14; fwrite(&software_size, 4, 1, file);
    fwrite("audio2silk", 1, 10, file); unsigned char padding[4] = {0, 0, 0, 0}; fwrite(padding, 1, 4, file);
    fwrite("data", 1, 4, file); fwrite(&data_size, 4, 1, file);
    for (uint32_t i = 0; i < frames; ++i) { int16_t sample = (int16_t)((i % 40) * 400); fwrite(&sample, 2, 1, file); }
    fclose(file);
}

int main(void) {
    const wchar_t *input = L"input.wav";
    const wchar_t *output = L"test-output.silk";
    if (_waccess(input, 0) != 0) {
        write_test_wav(input);
    }
    assert(convert_file(input, output) == CONVERT_OK);
    FILE *file = _wfopen(output, L"rb");
    assert(file != NULL);
    unsigned char header[10] = {0};
    assert(fread(header, 1, sizeof(header), file) == sizeof(header));
    assert(header[0] == 0x02);
    assert(memcmp(header + 1, "#!SILK_V3", 9) == 0);
    fclose(file);
    _wremove(output);
    return 0;
}
