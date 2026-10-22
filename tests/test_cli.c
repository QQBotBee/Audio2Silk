#include <assert.h>
#include <wchar.h>

#include "cli.h"

static void test_requires_exactly_two_paths(void) {
    wchar_t *argv[] = {L"audio2silk", L"input.wav", L"output.silk"};
    CliOptions options = {0};
    assert(parse_cli(3, argv, &options) == CLI_OK);
    assert(options.mode == CLI_MODE_ENCODE);
    assert(options.input_path == argv[1]);
    assert(options.output_path == argv[2]);
}

static void test_decode_flag_selects_decode_mode(void) {
    wchar_t *argv[] = {L"audio2silk", L"-d", L"input.silk", L"output.wav"};
    CliOptions options = {0};
    assert(parse_cli(4, argv, &options) == CLI_OK);
    assert(options.mode == CLI_MODE_DECODE);
    assert(options.input_path == argv[2]);
    assert(options.output_path == argv[3]);
}

static void test_rejects_missing_or_extra_paths(void) {
    wchar_t *empty[] = {NULL};
    wchar_t *missing[] = {L"audio2silk", L"input.wav"};
    wchar_t *extra[] = {L"audio2silk", L"input.wav", L"output.silk", L"extra"};
    wchar_t *decode_missing[] = {L"audio2silk", L"-d", L"input.silk"};
    wchar_t *unknown_flag[] = {L"audio2silk", L"-x", L"input.wav", L"output.silk"};
    CliOptions options = {0};
    assert(parse_cli(0, empty, &options) == CLI_INVALID_ARGUMENTS);
    assert(parse_cli(2, missing, &options) == CLI_INVALID_ARGUMENTS);
    assert(parse_cli(4, extra, &options) == CLI_INVALID_ARGUMENTS);
    assert(parse_cli(3, decode_missing, &options) == CLI_INVALID_ARGUMENTS);
    assert(parse_cli(4, unknown_flag, &options) == CLI_INVALID_ARGUMENTS);
}

int main(void) {
    test_requires_exactly_two_paths();
    test_decode_flag_selects_decode_mode();
    test_rejects_missing_or_extra_paths();
    return 0;
}
