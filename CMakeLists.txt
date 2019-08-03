cmake_minimum_required(VERSION 3.5)

if (NOT DynamoRIO_BUILD_DIR)
  message (FATAL_ERROR "You have to specify the build directory of DynamoRIO")
endif (NOT DynamoRIO_BUILD_DIR)

set(DynamoRIO_DIR ${DynamoRIO_BUILD_DIR}/cmake)
option(ENABLE_TESTS "Enable tests for the client" ON)


find_program(GENGETOPT_BIN gengetopt)
if (NOT GENGETOPT_BIN)
    message (FATAL_ERROR "The program 'gengetopt' was not found")
endif (NOT GENGETOPT_BIN)

# Specify the directory where we generate command-line interface.
set(CLI_GEN_DIR ${CMAKE_CURRENT_BINARY_DIR}/cli_gen)
file(MAKE_DIRECTORY ${CLI_GEN_DIR})

# Generate command-line interface through GNU gengetopt.
add_custom_command(
    OUTPUT ${CLI_GEN_DIR}/cmdline.h ${CLI_GEN_DIR}/cmdline.c
    COMMAND ${GENGETOPT_BIN} -i ${CMAKE_CURRENT_SOURCE_DIR}/src/cli.ggo
    --output-dir=${CLI_GEN_DIR}
    DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/src/cli.ggo
)
file(GLOB src_files src/*.cpp)

add_library(fsracer SHARED ${src_files}
    ${CLI_GEN_DIR}/cmdline.h
    ${CLI_GEN_DIR}/cmdline.c)
target_include_directories(fsracer PRIVATE ${CLI_GEN_DIR})

set(CMAKE_CXX_FLAGS  "-std=c++17 -lstdc++fs")
target_link_libraries(fsracer stdc++fs)

find_package(DynamoRIO REQUIRED)

use_DynamoRIO_extension(fsracer "drwrap")
use_DynamoRIO_extension(fsracer "drmgr")
use_DynamoRIO_extension(fsracer "drsyms")

configure_DynamoRIO_client(fsracer)

if (ENABLE_TESTS)
    enable_testing()
    add_subdirectory(tests)
endif (ENABLE_TESTS)
