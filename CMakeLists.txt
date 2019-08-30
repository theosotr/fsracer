cmake_minimum_required(VERSION 3.5)
option(ENABLE_TESTS "Enable tests for the client" ON)

add_subdirectory(lib)
add_subdirectory(tools)

if (ENABLE_TESTS)
    enable_testing()
    add_subdirectory(tests)
endif (ENABLE_TESTS)
