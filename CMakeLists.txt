cmake_minimum_required(VERSION 3.5)

add_library(fsracer SHARED src/fsracer.cpp src/tracer.cpp)

find_package(DynamoRIO REQUIRED HINTS ${DynamoRIO_BUILD_DIR})

use_DynamoRIO_extension(fsracer "drwrap")
use_DynamoRIO_extension(fsracer "drmgr")
use_DynamoRIO_extension(fsracer "drsyms")

configure_DynamoRIO_client(fsracer)
