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

if (NOT EXISTS ${test_file})
  message(FATAL_ERROR "The program under test ${test_file} does not exist")
endif ()

# We read the program under test and we add it some necessary code
# to enable testing.
file(READ ${test_file} program)
string(CONCAT program
  "const ah = require('async_hooks');\n"
  "function init() {  }\n"
  "function before() {  }\n"
  "function after() {  }\n"
  "function destroy() {  }\n"
  "function resolve() {  }\n"
  "ah.createHook({ init, before, after, destroy, resolve }).enable()\n"
  ${program}
)
# We save the instrumented program to a new temporary file.
file(WRITE ${TEST_FILE} ${program})
set(test_file ${TEST_FILE})

# Define the command to run.
set (cmd ${DYNAMO_FILE} -c ../libfsracer.so -- ${TEST_CMD} ${test_file})

# Running Test Application using DynamoRIO and capturing its standard output
# to a file.
execute_process(
  COMMAND ${cmd}
  OUTPUT_VARIABLE output
)
string(REGEX REPLACE "\n" "." output_rep ${output})

# We replace any boiler plate operations performed
# by Node during the the start of the program.
# Typically those operations are simple stat and lstat
# operations that check whether the given program
# and its parent directories exist.
string(CONCAT prologue_operation_repl
  "(Operation sync_@NUM@ do.done.)+"
  "Operation sync_@NUM@ do."
  "hpath AT_FDCWD /home[^\n]*/${TEST_FILE} consumed." # At this point Node opens the test program.
  "newFd AT_FDCWD /home/[^\n]*/${TEST_FILE} @NUM@."
  "done."
  "Operation sync_@NUM@ do."
  "delFd @NUM@."
  "done"
)

string(CONCAT prologue_block_repl
  "newEvent 2 EXTERNAL."
  "link 1 2."
  "newEvent 3 EXTERNAL."
  "link 1 3."
  "submitOp sync_1 stat SYNC."
  "(submitOp sync_@NUM@ lstat SYNC.)*"
  "submitOp sync_@NUM@ open SYNC."
  "submitOp sync_@NUM@ close SYNC"
)

if (EXISTS "${expected_filename}")
  # Opens the pattern file.
  file(READ ${expected_filename} pattern)

  #We replace boilerplate patterns.
  string(REGEX REPLACE
    "@PROLOGUE_OPERATIONS@"
    ${prologue_operation_repl}
    pattern
    ${pattern}
  )
  string(REGEX REPLACE
    "@PROLOGUE_BLOCK@"
    ${prologue_block_repl}
    pattern
    ${pattern}
  )
  string(REGEX REPLACE "@NUM@" "[0-9]+" pattern ${pattern})
  string(REGEX REPLACE "\n" "." pattern ${pattern})
  string(REGEX MATCH ${pattern} match ${output_rep})


  if (NOT match)
    file(WRITE ${TEST_FILE}.out ${output})
    message(SEND_ERROR
      "Test ${TEST_FILE} does not produce the expected output.\
      View resulting output at ${TEST_FILE}.out.")

  endif (NOT match)
else ()
  # If the file does not exist, we just create the expected file
  # and pass the current test.
  # Note that this can be used for creating expected files efficiently.
  file(WRITE "${expected_filename}" ${output})
endif ()

# We remove the temporary test file.
file(REMOVE ${test_file})
