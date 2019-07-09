cmake_minimum_required(VERSION 3.5)

if (NOT DynamoRIO_BUILD_DIR)
  message (FATAL_ERROR "You have to specify the build directory of DynamoRIO")
endif (NOT DynamoRIO_BUILD_DIR)

set(DynamoRIO_DIR ${DynamoRIO_BUILD_DIR}/cmake)
option(ENABLE_TESTS "Enable tests for the client" ON)

file(GLOB src_files src/*.cpp)
add_library(fsracer SHARED ${src_files})

find_package(DynamoRIO REQUIRED)

use_DynamoRIO_extension(fsracer "drwrap")
use_DynamoRIO_extension(fsracer "drmgr")
use_DynamoRIO_extension(fsracer "drsyms")

configure_DynamoRIO_client(fsracer)

if (ENABLE_TESTS)
    enable_testing()
    add_subdirectory(tests)
endif (ENABLE_TESTS)
