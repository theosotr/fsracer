cmake_minimum_required(VERSION 3.5)

file(GLOB src_files src/*.cpp)
add_library(fsracer SHARED ${src_files})

find_package(DynamoRIO REQUIRED HINTS ${DynamoRIO_BUILD_DIR})

use_DynamoRIO_extension(fsracer "drwrap")
use_DynamoRIO_extension(fsracer "drmgr")
use_DynamoRIO_extension(fsracer "drsyms")

configure_DynamoRIO_client(fsracer)
