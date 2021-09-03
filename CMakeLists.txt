cmake_minimum_required(VERSION 3.20)
project(gdpc)

# Command line options
option(DEBUG "Compile target for debugging (ON | OFF)" ON)

file(GLOB GDPC_SOURCE_FILES "src/*.c")
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/../bin)
add_executable(gdpc ${GDPC_SOURCE_FILES})

# Set compiler options
if(DEBUG)
    SET(CMAKE_C_FLAGS "-g -O0 -std=c99 -Wall -Wextra -Wpedantic -Werror -fsanitize=address")
    SET(CMAKE_EXE_LINKER_FLAGS "-fsanitize=address")
else()
    SET(CMAKE_C_FLAGS "-03 -std=c99")
endif()
