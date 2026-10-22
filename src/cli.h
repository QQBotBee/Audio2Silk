#ifndef AUDIO2SILK_CLI_H
#define AUDIO2SILK_CLI_H

#include <wchar.h>

typedef enum {
    CLI_MODE_ENCODE = 0,
    CLI_MODE_DECODE = 1
} CliMode;

typedef struct {
    CliMode mode;
    const wchar_t *input_path;
    const wchar_t *output_path;
} CliOptions;

typedef enum {
    CLI_OK = 0,
    CLI_INVALID_ARGUMENTS = 2
} CliStatus;

CliStatus parse_cli(int argc, wchar_t **argv, CliOptions *options);
void print_usage(const wchar_t *program_name);

#endif
