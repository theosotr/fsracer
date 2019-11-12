cmake_minimum_required(VERSION 3.5)

find_package(nlohmann_json REQUIRED)

file(GLOB src_files *.cpp)

add_library(fsracer-lib STATIC ${src_files})
set(CMAKE_CXX_FLAGS  "-std=c++17 -lstdc++fs")
set_property(TARGET fsracer-lib PROPERTY POSITION_INDEPENDENT_CODE ON)
