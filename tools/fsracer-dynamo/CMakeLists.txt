cmake_minimum_required(VERSION 3.5)

if (NOT DynamoRIO_BUILD_DIR)
  message (FATAL_ERROR "You have to specify the build directory of DynamoRIO")
endif (NOT DynamoRIO_BUILD_DIR)

set(DynamoRIO_DIR ${DynamoRIO_BUILD_DIR}/cmake)


find_program(GENGETOPT_BIN gengetopt)
if (NOT GENGETOPT_BIN)
    message (FATAL_ERROR "The program 'gengetopt' was not found")
endif (NOT GENGETOPT_BIN)

# Specify the directory where we generate command-line interface.
set(CLI_GEN_DIR ${CMAKE_CURRENT_BINARY_DIR}/cli_gen)
file(MAKE_DIRECTORY ${CLI_GEN_DIR})

# Generate command-line interface through GNU gengetopt.
add_custom_command(
    OUTPUT ${CLI_GEN_DIR}/fsracer_cli.h ${CLI_GEN_DIR}/fsracer_cli.c
    COMMAND ${GENGETOPT_BIN} -i ${CMAKE_CURRENT_SOURCE_DIR}/fsracer_cli.ggo
    --output-dir=${CLI_GEN_DIR}
    --file-name=fsracer_cli
    DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/fsracer_cli.ggo
)
file(GLOB src_files ${CMAKE_SOURCE_DIR}/src/*.cpp)

add_library(fsracer SHARED fsracer.cpp
  ${src_files}
  ${CLI_GEN_DIR}/fsracer_cli.h
  ${CLI_GEN_DIR}/fsracer_cli.c)
target_include_directories(fsracer PRIVATE ${CLI_GEN_DIR})
target_include_directories(fsracer PRIVATE ${CMAKE_SOURCE_DIR}/src)

set(CMAKE_CXX_FLAGS  "-std=c++17 -lstdc++fs")
target_link_libraries(fsracer stdc++fs)

find_package(DynamoRIO REQUIRED)

use_DynamoRIO_extension(fsracer "drwrap")
use_DynamoRIO_extension(fsracer "drmgr")
use_DynamoRIO_extension(fsracer "drsyms")

configure_DynamoRIO_client(fsracer)
