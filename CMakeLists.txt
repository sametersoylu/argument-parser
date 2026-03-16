cmake_minimum_required(VERSION 3.15)

project(argument_parser VERSION 1.0)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/bin)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY_Release ${CMAKE_CURRENT_SOURCE_DIR}/bin/release)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY_Debug ${CMAKE_CURRENT_SOURCE_DIR}/bin/debug)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

file(GLOB_RECURSE SRC_FILES "src/source/*.cpp" "src/source/**/*.cpp" "src/source/**/**/*.cpp")

add_library(argument_parser ${SRC_FILES})

include(GNUInstallDirs)

target_include_directories(argument_parser PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/src/headers>
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/src/headers/parser>
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/src/headers/conventions>
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/src/headers/conventions/implementations>
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/src/headers/parser/platform_headers>
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/src/headers/parser/parsing_traits>
    $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>
    $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}/parser>
    $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}/conventions>
    $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}/conventions/implementations>
    $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}/parser/platform_headers>
    $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}/parser/parsing_traits>
)

install(TARGETS argument_parser
    EXPORT argument_parserTargets
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
    INCLUDES DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}
)

install(DIRECTORY src/headers/
    DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}
)

install(EXPORT argument_parserTargets
    FILE argument_parserTargets.cmake
    NAMESPACE argument_parser::
    DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/argument_parser
)

add_executable(test src/main.cpp)
target_link_libraries(test PRIVATE argument_parser)