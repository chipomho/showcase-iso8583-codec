cmake_minimum_required(VERSION 3.25)

project(iso8583_codec
    VERSION 1.0.0
    DESCRIPTION "Encoder and decoder for ISO 8583:1987 card payment messages"
    LANGUAGES CXX)

set(CMAKE_CXX_EXTENSIONS OFF)

option(ISO8583_BUILD_TESTS "Build the unit tests" ${PROJECT_IS_TOP_LEVEL})
option(ISO8583_COVERAGE "Instrument for gcov and add a 'coverage' target (GCC only)" OFF)
set(ISO8583_SANITIZERS "" CACHE STRING "Sanitizers to build with, e.g. address;undefined (GCC/Clang)")

include(cmake/CompilerOptions.cmake)

add_library(iso8583_codec
    src/bitmap.cpp
    src/bytes.cpp
    src/codec.cpp
    src/field_spec.cpp
    src/message.cpp
    src/mti.cpp
    src/print.cpp
    src/result.cpp)
add_library(iso8583::codec ALIAS iso8583_codec)
target_include_directories(iso8583_codec
    PUBLIC ${PROJECT_SOURCE_DIR}/include
    PRIVATE ${PROJECT_SOURCE_DIR}/src)
target_compile_features(iso8583_codec PUBLIC cxx_std_20)
target_link_libraries(iso8583_codec PRIVATE iso8583_warnings)

# The command line tool's logic lives in a library so the tests can drive it
# with string streams; main.cpp only adapts argv and the standard streams.
add_library(iso8583_cli STATIC apps/iso8583/cli.cpp)
target_include_directories(iso8583_cli PUBLIC ${PROJECT_SOURCE_DIR}/apps/iso8583)
target_link_libraries(iso8583_cli PUBLIC iso8583::codec PRIVATE iso8583_warnings)

add_executable(iso8583 apps/iso8583/main.cpp)
target_link_libraries(iso8583 PRIVATE iso8583_cli iso8583_warnings)

if(ISO8583_BUILD_TESTS)
    enable_testing()
    add_subdirectory(tests)
endif()
