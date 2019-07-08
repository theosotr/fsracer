cmake_minimum_required(VERSION 3.5)


# Set the path to the binary that we test (e.g., path to node binary).
set(TEST_BINARY_PATH "" CACHE STRING "Path to the test binary")

# Build FSracer before running the tests.
# TODO: Check if there is another workround.
add_test(build_fsracer
        "${CMAKE_COMMAND}" --build ${CMAKE_BINARY_DIR} --target fsracer)

function (new_test test_suite test_name test_file)
  # Actual tests
  add_test(${test_name}
      ${CMAKE_COMMAND}
      -D TEST_CMD=${TEST_BINARY_PATH}
      -D TEST_FILE=${test_file}
      -D TEST_SUITE=${test_suite}
      -D DYNAMO_FILE=/home/thodoris/software/dynamo/bin64/drrun #FIXME: Make the path to dynamo configurable
      -P "${CMAKE_SOURCE_DIR}/tests/runtest.cmake"
  )
  set_tests_properties(${test_name} PROPERTIES DEPENDS build_fsracer)
endfunction (new_test)


new_test (node_tests test_timers timers.js)
