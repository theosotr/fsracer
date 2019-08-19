cmake_minimum_required(VERSION 3.5)


# Set the path to the binary that we test (e.g., path to node binary).
set(TEST_BINARY_PATH "" CACHE STRING "Path to the test binary")

# Build FSracer before running the tests.
# TODO: Check if there is another workround.
add_test(build_fsracer
        "${CMAKE_COMMAND}" --build ${CMAKE_BINARY_DIR} --target fsracer)

message("${DynamoRIO_BUILD_DIR}")

string(CONCAT cmd_options
  "--dep-graph-format=csv "
  "--dump-dep-graph "
  "--fs-accesses-format=csv "
  "--dump-fs-accesses "
  "--fault-detector=race "
  "--dump-trace"
)
set(DEFAULT_FSRACER_ARGS ${cmd_options})
function (new_test test_suite test_name test_file)
  # Actual tests
  add_test(${test_name}
      ${CMAKE_COMMAND}
      -D TEST_CMD=${TEST_BINARY_PATH}
      -D TEST_FILE=${test_file}
      -D TEST_SUITE=${test_suite}
      -D DYNAMO_FILE=${DynamoRIO_BUILD_DIR}/bin64/drrun
      -D FSRACER_ARGS=${DEFAULT_FSRACER_ARGS}
      -P "${CMAKE_SOURCE_DIR}/tests/runtest.cmake"
  )
  set_tests_properties(${test_name} PROPERTIES DEPENDS build_fsracer)
endfunction (new_test)


new_test (node_tests test_timers timers.js)
new_test (node_tests test_compound compound.js)
new_test (node_tests test_promises promises.js)
new_test (node_tests test_promise-thenable promise-thenable.js)
new_test (node_tests test_promise-io promise-io.js)
new_test (node_tests test_promise-chain promise-chain.js)
new_test (node_tests test_timers-nested timers-nested.js)
new_test (node_tests test_timers-nested2 timers-nested2.js)
new_test (node_tests test_io-nested io-nested.js)
new_test (node_tests test_timers-nested-err timers-nested-err.js)
new_test (node_tests test_thenables thenables.js)
new_test (node_tests test_timers-different-timeout timers-different-timeout.js)
new_test (node_tests test_next-tick next-tick.js)
new_test (node_tests test_http http.js)
new_test (node_tests test_stream stream.js)
new_test (node_tests test_immediate-timeout-tick immediate-timeout-tick.js)
