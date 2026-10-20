include(FetchContent)

FetchContent_Declare(googletest
    URL https://github.com/google/googletest/releases/download/v1.17.0/googletest-1.17.0.tar.gz
    URL_HASH SHA256=65fab701d9829d38cb77c14acdc431d2108bfdbf8979e40eb8ae567edf10b27c
    SYSTEM)
set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)
set(BUILD_GMOCK OFF CACHE BOOL "" FORCE)
set(INSTALL_GTEST OFF CACHE BOOL "" FORCE)
FetchContent_MakeAvailable(googletest)

add_executable(iso8583_tests
    bitmap_test.cpp
    bytes_test.cpp
    cli_test.cpp
    decode_test.cpp
    encode_test.cpp
    field_spec_test.cpp
    message_test.cpp
    mti_test.cpp
    print_test.cpp
    result_test.cpp
    round_trip_test.cpp)
target_link_libraries(iso8583_tests PRIVATE iso8583::codec iso8583_cli GTest::gtest_main
                                            iso8583_warnings)

# In a coverage build every test run starts from zeroed counters, so the
# coverage gate only ever credits the tests that exist now.
set(iso8583_test_properties)
if(ISO8583_COVERAGE)
    add_test(NAME coverage.reset_counters
             COMMAND ${CMAKE_COMMAND} -DDIR=${PROJECT_BINARY_DIR}
                     -P ${PROJECT_SOURCE_DIR}/cmake/ResetCoverageCounters.cmake)
    set_tests_properties(coverage.reset_counters PROPERTIES FIXTURES_SETUP coverage_counters)
    set(iso8583_test_properties PROPERTIES FIXTURES_REQUIRED coverage_counters)
endif()

include(GoogleTest)
gtest_discover_tests(iso8583_tests DISCOVERY_MODE PRE_TEST ${iso8583_test_properties})

# End to end through the real executable: `sample` output fed to `decode`.
add_test(NAME cli.sample_decodes
         COMMAND ${CMAKE_COMMAND} -DCLI=$<TARGET_FILE:iso8583>
                 -P ${CMAKE_CURRENT_SOURCE_DIR}/cli_sample_decodes.cmake)
if(ISO8583_COVERAGE)
    set_tests_properties(cli.sample_decodes PROPERTIES FIXTURES_REQUIRED coverage_counters)
endif()

# The compile-time checks only count if bad input really fails to compile.
# Each case builds a target that must be rejected, and the build log must name
# the reason, so an unrelated build failure cannot pass for the right one.
set(compile_fail_bad_mti_literal "mti_literal_must_be_four_digits")
set(compile_fail_bad_field_table "field_table_is_invalid")
foreach(case IN ITEMS bad_mti_literal bad_field_table)
    add_executable(compile_fail_${case} EXCLUDE_FROM_ALL compile_fail/${case}.cpp)
    target_link_libraries(compile_fail_${case} PRIVATE iso8583::codec)
    add_test(NAME compile_fail.${case}
             COMMAND ${CMAKE_COMMAND} --build ${PROJECT_BINARY_DIR} --target compile_fail_${case})
    set_tests_properties(compile_fail.${case} PROPERTIES
        PASS_REGULAR_EXPRESSION "${compile_fail_${case}}"
        RESOURCE_LOCK build_tree)
endforeach()
