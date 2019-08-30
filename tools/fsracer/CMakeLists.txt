cmake_minimum_required(VERSION 3.5)

find_package(FLEX REQUIRED)
find_package(BISON REQUIRED)

flex_target(scanner scanner.l ${CMAKE_SOURCE_DIR}/tools/fsracer/scanner.cpp)
bison_target(parser parser.y ${CMAKE_SOURCE_DIR}/tools/fsracer/parser.cpp)
add_flex_bison_dependency(scanner parser)
include_directories(${CMAKE_SOURCE_DIR}/lib)
link_directories(${CMAKE_SOURCE_DIR}/lib)

set(CMAKE_INCLUDE_CURRENT_DIR ON)
file(GLOB src_files *.cpp *.cc)

add_executable(fsracer-exe
  ${src_files}
  ${BISON_parser_OUTPUTS}
  ${FLEX_scanner_OUTPUTS})
target_link_libraries(fsracer-exe fsracer-lib)
set(CMAKE_CXX_FLAGS  "-std=c++17 -lstdc++fs -fPIC")
target_link_libraries(fsracer-exe stdc++fs)
