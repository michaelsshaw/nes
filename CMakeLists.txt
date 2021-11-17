cmake_minimum_required(VERSION 3.16.3)
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR})
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR})
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR})

file(GLOB_RECURSE RESS res/*)
file(COPY ${RESS} DESTINATION ${CMAKE_RUNTIME_OUTPUT_DIRECTORY})

include_directories(include)
include_directories(lib/SDL2/include)
include_directories(lib/6502/include)

project(project_NES)


file(GLOB_RECURSE SRCS src/*.c)

add_executable(NES ${SRCS})

target_link_libraries(NES m)
if(NOT WIN32)
target_link_libraries(NES dl)
find_package(SDL2 REQUIRED)
endif(NOT WIN32)

if(WIN32)
    add_subdirectory(lib/SDL2)
    set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -lwinpthread")
endif(WIN32)

add_subdirectory(lib/6502)
target_link_libraries(NES SDL2 ${SDL2_LIBRARIES})
target_link_libraries(NES 6502 ${6502_LIBRARIES})