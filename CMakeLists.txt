cmake_minimum_required(VERSION 3.16.3)
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR})
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR})
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR})

set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${CMAKE_SOURCE_DIR}/util/cmake/")

file(GLOB_RECURSE RESS res/*)
file(GLOB_RECURSE ROMS tests/*.nes)
file(COPY ${RESS} DESTINATION ${CMAKE_RUNTIME_OUTPUT_DIRECTORY})
file(COPY ${ROMS} DESTINATION ${CMAKE_RUNTIME_OUTPUT_DIRECTORY})

set(CMAKE_C_FLAGS "-O3 -Wall -std=gnu11 -pg -no-pie -fno-builtin")

include_directories(include)
include_directories(lib/SDL2/include)
include_directories(lib/6502/include)

project(project_nes)


file(GLOB_RECURSE SRCS src/*.c)

add_executable(nes ${SRCS})

find_package(wxWidgets COMPONENTS gl core base)

target_link_libraries(nes m)
if(NOT WIN32 AND NOT APPLE)
    target_link_libraries(nes dl)
    set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -pthread")
    
    find_package(SDL2 REQUIRED)
endif(NOT WIN32 AND NOT APPLE)

if(WIN32)
    add_subdirectory(lib/SDL2)
    set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -lwinpthread")
endif(WIN32)

add_subdirectory(lib/6502)
target_link_libraries(nes SDL2 ${SDL2_LIBRARIES})
target_link_libraries(nes 6502 ${6502_LIBRARIES})

find_package(Curses REQUIRED)
include_directories(${CURSES_INCLUDE_DIR})

target_link_libraries(nes ${CURSES_LIBRARIES})
