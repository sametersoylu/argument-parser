cmake_minimum_required(VERSION 3.15)

project(argument_parser)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/bin)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY_Release ${CMAKE_CURRENT_SOURCE_DIR}/bin/release)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY_Debug ${CMAKE_CURRENT_SOURCE_DIR}/bin/debug)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

include_directories(include)
include_directories(include/parser)
include_directories(include/conventions)
include_directories(include/conventions/implementations)
include_directories(include/parser/platform_headers)
include_directories(include/parser/parsing_traits)

add_executable(test src/main.cpp)