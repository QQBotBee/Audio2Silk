#include "cli.h"
#include "convert.h"
#include "silk_decoder.h"

#include <stdio.h>

int wmain(int argc, wchar_t **argv) {
    CliOptions options;
    const CliStatus status = parse_cli(argc, argv, &options);
    if (status != CLI_OK) {
        print_usage(argc > 0 ? argv[0] : L"audio2silk");
        return status;
    }
    const ConvertStatus result = options.mode == CLI_MODE_DECODE
                                     ? decode_file(options.input_path, options.output_path)
                                     : convert_file(options.input_path, options.output_path);
    if (result != CONVERT_OK) {
        fprintf(stderr, "operation failed: %d, input length=%zu, output length=%zu\n",
                (int)result, wcslen(options.input_path), wcslen(options.output_path));
        return result;
    }
    return 0;
}
