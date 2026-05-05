cmake_minimum_required(VERSION 3.10)

project(test)

set(CMAKE_BUILD_TYPE Debug)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

find_package(argument_parser REQUIRED)
add_executable(main main.cpp)
target_link_libraries(main argument_parser)
