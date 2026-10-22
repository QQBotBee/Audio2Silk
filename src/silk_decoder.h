#ifndef AUDIO2SILK_SILK_DECODER_H
#define AUDIO2SILK_SILK_DECODER_H

#include "convert.h"

#include <wchar.h>

ConvertStatus decode_file(const wchar_t *input_path, const wchar_t *output_path);

#endif
