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
    OUTPUT ${CLI_GEN_DIR}/drfsracer_cli.h ${CLI_GEN_DIR}/drfsracer_cli.c
    COMMAND ${GENGETOPT_BIN} -i ${CMAKE_CURRENT_SOURCE_DIR}/drfsracer_cli.ggo
    --output-dir=${CLI_GEN_DIR}
    --file-name=drfsracer_cli
    DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/drfsracer_cli.ggo
)

include_directories(${CMAKE_SOURCE_DIR}/lib)
link_directories(${CMAKE_SOURCE_DIR}/lib)
file(GLOB src_files *.cpp)

add_library(drfsracer SHARED drfsracer.cpp
  ${src_files}
  ${CLI_GEN_DIR}/drfsracer_cli.h
  ${CLI_GEN_DIR}/drfsracer_cli.c)
target_include_directories(drfsracer PRIVATE ${CLI_GEN_DIR})
target_link_libraries(drfsracer fsracer-lib)

set(CMAKE_CXX_FLAGS  "-std=c++17 -lstdc++fs -fPIC")
target_link_libraries(drfsracer stdc++fs)

find_package(DynamoRIO REQUIRED)

use_DynamoRIO_extension(drfsracer "drwrap")
use_DynamoRIO_extension(drfsracer "drmgr")
use_DynamoRIO_extension(drfsracer "drsyms")

configure_DynamoRIO_client(drfsracer)
