cmake_minimum_required(VERSION 3.5)

file(GLOB src_files *.cpp)

add_library(fsracer-lib STATIC ${src_files})
set(CMAKE_CXX_FLAGS  "-std=c++17 -lstdc++fs -pg")
target_link_libraries(fsracer-lib stdc++fs)
set_property(TARGET fsracer-lib PROPERTY POSITION_INDEPENDENT_CODE ON)
