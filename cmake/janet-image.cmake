execute_process(COMMAND "${JANET_EXECUTABLE}" "${JANET_ROOT}" image-only
    OUTPUT_FILE "${OUTPUT_FILE}" RESULT_VARIABLE status)
if(NOT status EQUAL 0)
    message(FATAL_ERROR "Janet image generation failed: ${status}")
endif()
