if (NOT DYNAMO_FILE)
  message (FATAL_ERROR "Variable `DYNAMO_FILE` must be defined")
endif (NOT DYNAMO_FILE)

if (NOT TEST_CMD)
  message (FATAL_ERROR "Variable `TEST_CMD` must be defined")
endif (NOT TEST_CMD)

if (NOT TEST_FILE)
  message (FATAL_ERROR "Variable `TEST_FILE` must be defined")
endif (NOT TEST_FILE)

if (NOT TEST_SUITE)
  message (FATAL_ERROR "Variable `TEST_SUITE` must be defined")
endif (NOT TEST_SUITE)


message("Testing with DynamoRIO file: ${DYNAMO_FILE}")
message("Test Binary: ${TEST_CMD}")

# Determine the filename tha contains the expected output.
string(REPLACE ".js" ".exp" filename ${TEST_FILE})
# Get the absolute path of the file that holds the expected result.
get_filename_component(expected_filename
  "${CMAKE_SOURCE_DIR}/../../tests/${TEST_SUITE}/${filename}"
  REALPATH)
# Get the absolute path of the file that contains the source
# code of the program under test.
get_filename_component(test_file
  "${CMAKE_SOURCE_DIR}/../../tests/${TEST_SUITE}/${TEST_FILE}"
  REALPATH)

# Define the command to run.
set (cmd ${DYNAMO_FILE} -c ../libfsracer.so -- ${TEST_CMD} ${test_file})

# Running Test Application using DynamoRIO and capturing its standard output
# to a file.
execute_process(
  COMMAND ${cmd}
  OUTPUT_VARIABLE output
)

# Replace PID with any empty string.
string(REGEX REPLACE
  "[a-zA-Z]+: PID [0-9]+, Start collecting trace..."
  "Start collecting trace..."
  output_repl
  ${output}
)

# Replace any absolute paths produced by the tool.
string(REGEX REPLACE
  "/home/[^\n]*/${TEST_FILE}"
  "${TEST_FILE}"
  output_repl
  ${output_repl}
)
file(WRITE output_file ${output_repl})


if (EXISTS "${expected_filename}")
  # The filename exists, so we use the `diff` utility to compare
  # the expected result with the current one.
  execute_process(
    COMMAND diff output_file ${expected_filename}
    OUTPUT_VARIABLE output
    RESULT_VARIABLE exit_code
  )

  # The `diff` utility produced a non zero-exit code, so we presume
  # that the files do not match.
  if (exit_code)
    message(SEND_ERROR
      "Test ${TEST_FILE} does not produce the expected output\n${output}")
  endif (exit_code) 
else ()
  # If the file does not exist, we just create the expected file
  # and pass the current test.
  # Note that this can be used for creating expected files efficiently.
  file(WRITE "${expected_filename}" ${output_repl})
endif ()
