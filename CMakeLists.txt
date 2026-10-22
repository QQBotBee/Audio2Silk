cmake_minimum_required(VERSION 3.20)
project(audio2silk C)

set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)

enable_testing()

file(GLOB SILK_SOURCES CONFIGURE_DEPENDS third_party/silk/src/*.c)

add_library(audio2silk_core STATIC
    src/cli.c
    src/convert.c
    src/silk_decoder.c
    src/silk_encoder.c
    src/miniaudio_impl.c
    ${SILK_SOURCES}
)
target_include_directories(audio2silk_core PUBLIC src third_party/miniaudio third_party/stb third_party/silk/interface third_party/silk/src)
if (WIN32)
    target_compile_definitions(audio2silk_core PRIVATE _CRT_SECURE_NO_WARNINGS)
endif()

add_executable(test_cli tests/test_cli.c)
target_link_libraries(test_cli PRIVATE audio2silk_core)
add_test(NAME cli COMMAND test_cli)

add_executable(test_silk_encoder tests/test_silk_encoder.c)
target_link_libraries(test_silk_encoder PRIVATE audio2silk_core)
add_test(NAME silk_encoder COMMAND test_silk_encoder)

add_executable(test_convert tests/test_convert.c)
target_link_libraries(test_convert PRIVATE audio2silk_core)
add_test(NAME convert COMMAND test_convert)

add_executable(test_decode tests/test_decode.c)
target_link_libraries(test_decode PRIVATE audio2silk_core)
add_test(NAME decode COMMAND test_decode)

foreach(test_target test_cli test_silk_encoder test_convert test_decode)
    if(MSVC)
        target_compile_options(${test_target} PRIVATE /UNDEBUG)
    else()
        target_compile_options(${test_target} PRIVATE -UNDEBUG)
    endif()
endforeach()

add_executable(audio2silk src/main.c)
target_link_libraries(audio2silk PRIVATE audio2silk_core)
if (MINGW)
    target_link_options(audio2silk PRIVATE -municode)
endif()
