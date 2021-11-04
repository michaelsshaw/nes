cmake_minimum_required(VERSION 3.16.3)
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR})
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR})
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR})

include_directories(include)
include_directories(lib/6502/include)

project(project_NES)

file(GLOB_RECURSE SRCS src/*.c)

add_executable(NES ${SRCS})

target_link_libraries(NES m)
if(NOT WIN32)
    target_link_libraries(NES dl)
endif(NOT WIN32)