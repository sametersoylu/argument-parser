cmake_minimum_required(VERSION 3.15)

project(argument_parser)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/bin)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY_Release ${CMAKE_CURRENT_SOURCE_DIR}/bin/release)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY_Debug ${CMAKE_CURRENT_SOURCE_DIR}/bin/debug)

add_executable(test src/main.cpp)