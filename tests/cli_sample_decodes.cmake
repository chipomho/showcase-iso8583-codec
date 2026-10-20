# Runs `iso8583 sample`, feeds its output to `iso8583 decode`, and checks the
# printed message masks the PAN. Invoked by CTest with -DCLI=<path to iso8583>.

execute_process(COMMAND "${CLI}" sample
                OUTPUT_VARIABLE hex
                RESULT_VARIABLE status
                OUTPUT_STRIP_TRAILING_WHITESPACE)
if(NOT status EQUAL 0)
    message(FATAL_ERROR "iso8583 sample exited with ${status}")
endif()

execute_process(COMMAND "${CLI}" decode "${hex}"
                OUTPUT_VARIABLE printed
                RESULT_VARIABLE status)
if(NOT status EQUAL 0)
    message(FATAL_ERROR "iso8583 decode exited with ${status}")
endif()
if(NOT printed MATCHES "411111\\*\\*\\*\\*\\*\\*1111")
    message(FATAL_ERROR "masked PAN missing from:\n${printed}")
endif()
if(printed MATCHES "4111111111111111")
    message(FATAL_ERROR "clear PAN printed:\n${printed}")
endif()

execute_process(COMMAND "${CLI}" decode 3032
                RESULT_VARIABLE status
                ERROR_VARIABLE error_text)
if(NOT status EQUAL 1)
    message(FATAL_ERROR "decode of a truncated message exited with ${status}, expected 1")
endif()

message(STATUS "sample decoded and printed with the PAN masked")
