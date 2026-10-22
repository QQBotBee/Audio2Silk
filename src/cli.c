#include "cli.h"

#include <stdio.h>

CliStatus parse_cli(int argc, wchar_t **argv, CliOptions *options) {
    if (!options || !argv || argc < 3) {
        return CLI_INVALID_ARGUMENTS;
    }

    int path_index = 1;
    CliMode mode = CLI_MODE_ENCODE;
    if (argv[1] && wcscmp(argv[1], L"-d") == 0) {
        if (argc != 4) {
            return CLI_INVALID_ARGUMENTS;
        }
        mode = CLI_MODE_DECODE;
        path_index = 2;
    } else if (argc != 3) {
        return CLI_INVALID_ARGUMENTS;
    }

    if (!argv[path_index] || !argv[path_index + 1] ||
        argv[path_index][0] == L'\0' || argv[path_index + 1][0] == L'\0') {
        return CLI_INVALID_ARGUMENTS;
    }
    options->mode = mode;
    options->input_path = argv[path_index];
    options->output_path = argv[path_index + 1];
    return CLI_OK;
}

void print_usage(const wchar_t *program_name) {
    const wchar_t *name = program_name ? program_name : L"audio2silk";
    fwprintf(stderr, L"用法: %ls <输入音频路径> <输出.silk路径>\n", name);
    fwprintf(stderr, L"      %ls -d <输入.silk路径> <输出.wav路径>\n", name);
}
