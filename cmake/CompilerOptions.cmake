# Warnings are errors for this project's own targets. Third-party code
# (GoogleTest) is compiled without them, so it never blocks a build.
add_library(iso8583_warnings INTERFACE)
target_compile_options(iso8583_warnings INTERFACE
    $<$<CXX_COMPILER_ID:MSVC>:/W4 /WX /permissive- /utf-8>
    $<$<CXX_COMPILER_ID:GNU,Clang,AppleClang>:-Wall -Wextra -Wpedantic -Werror -Wconversion -Wshadow>)

# Sanitizers apply to everything, GoogleTest included, so instrumented and
# uninstrumented code never mix.
if(ISO8583_SANITIZERS)
    if(MSVC)
        message(FATAL_ERROR "ISO8583_SANITIZERS is only wired up for GCC and Clang")
    endif()
    list(JOIN ISO8583_SANITIZERS "," iso8583_sanitizer_list)
    add_compile_options(-fsanitize=${iso8583_sanitizer_list} -fno-omit-frame-pointer
                        -fno-sanitize-recover=all)
    add_link_options(-fsanitize=${iso8583_sanitizer_list})
endif()

if(ISO8583_COVERAGE)
    if(NOT CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
        message(FATAL_ERROR "ISO8583_COVERAGE expects GCC and gcov")
    endif()
    add_compile_options(--coverage)
    add_link_options(--coverage)

    find_program(GCOVR_EXECUTABLE gcovr REQUIRED)
    set(iso8583_coverage_dir ${PROJECT_BINARY_DIR}/coverage)

    # Run after the tests. Fails the build when line coverage of the library,
    # its headers or the CLI drops below 100%.
    #
    # --merge-lines: an inline function compiled into several objects keeps
    #   one body after linking, but every object keeps its own counters. GCC 11
    #   gives member-initializer lines no function name, so without this the
    #   discarded copies' zero counts show as uncovered lines.
    # --exclude-noncode-lines: GCC attributes the exception cleanup of a
    #   returned local to the function's closing brace, which then only
    #   "executes" when an allocation throws.
    #
    # Branch coverage is reported, not gated: GCC counts every call that can
    # throw as a branch. The second summary shows it without those edges.
    set(iso8583_gcovr_common
        --root ${PROJECT_SOURCE_DIR}
        --object-directory ${PROJECT_BINARY_DIR}
        --filter ${PROJECT_SOURCE_DIR}/include/
        --filter ${PROJECT_SOURCE_DIR}/src/
        --filter ${PROJECT_SOURCE_DIR}/apps/
        --merge-lines
        --exclude-noncode-lines)
    add_custom_target(coverage
        COMMAND ${CMAKE_COMMAND} -E make_directory ${iso8583_coverage_dir}
        COMMAND ${GCOVR_EXECUTABLE} ${iso8583_gcovr_common}
                --txt
                --html-details ${iso8583_coverage_dir}/index.html
                --print-summary
                --fail-under-line 100
        COMMAND ${CMAKE_COMMAND} -E echo "Excluding exception-handling branches:"
        COMMAND ${GCOVR_EXECUTABLE} ${iso8583_gcovr_common}
                --exclude-throw-branches
                --txt ${iso8583_coverage_dir}/without-throw-branches.txt
                --print-summary
        WORKING_DIRECTORY ${PROJECT_BINARY_DIR}
        COMMENT "Measuring coverage (gate: 100% of lines)"
        VERBATIM)
endif()
