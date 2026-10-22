#ifndef AUDIO2SILK_CONVERT_H
#define AUDIO2SILK_CONVERT_H

#include <wchar.h>

typedef enum {
    CONVERT_OK = 0,
    CONVERT_INVALID_ARGUMENTS = 10,
    CONVERT_INPUT_OPEN_FAILED = 11,
    CONVERT_DECODE_FAILED = 12,
    CONVERT_ENCODE_FAILED = 13,
    CONVERT_OUTPUT_FAILED = 14
} ConvertStatus;

ConvertStatus convert_file(const wchar_t *input_path, const wchar_t *output_path);

#endif
