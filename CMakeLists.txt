cmake_minimum_required(VERSION 3.15)

project(argument_parser)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/bin)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY_Release ${CMAKE_CURRENT_SOURCE_DIR}/bin/release)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY_Debug ${CMAKE_CURRENT_SOURCE_DIR}/bin/debug)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

include_directories(src/headers)
include_directories(src/headers/parser)
include_directories(src/headers/conventions)
include_directories(src/headers/conventions/implementations)
include_directories(src/headers/parser/platform_headers)
include_directories(src/headers/parser/parsing_traits)

add_executable(test src/main.cpp)