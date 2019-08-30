cmake_minimum_required(VERSION 3.5)

find_package(FLEX REQUIRED)
find_package(BISON REQUIRED)

flex_target(scanner TraceLexer.l ${CMAKE_SOURCE_DIR}/tools/fsracer/TraceLexer.cpp)
bison_target(parser TraceParser.y ${CMAKE_SOURCE_DIR}/tools/fsracer/TraceParser.cpp)
add_flex_bison_dependency(scanner parser)


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

include_directories(${CMAKE_SOURCE_DIR}/lib)
link_directories(${CMAKE_SOURCE_DIR}/lib)

set(CMAKE_INCLUDE_CURRENT_DIR ON)
file(GLOB src_files *.cpp *.cc)

add_executable(fsracer-exe
  ${src_files}
  ${BISON_parser_OUTPUTS}
  ${FLEX_scanner_OUTPUTS}
  ${CLI_GEN_DIR}/fsracer_cli.h
  ${CLI_GEN_DIR}/fsracer_cli.c)

target_include_directories(fsracer-exe PRIVATE ${CLI_GEN_DIR})
target_link_libraries(fsracer-exe fsracer-lib)
set(CMAKE_CXX_FLAGS  "-std=c++17 -lstdc++fs -fPIC")
target_link_libraries(fsracer-exe stdc++fs)
