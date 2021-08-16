cmake_minimum_required(VERSION 3.20)
project(gdpc)

# Command line options
option(DEBUG "Compile target for debugging (ON | OFF)" ON)

file(GLOB GDPC_SOURCE_FILES "src/*.c")
add_executable(gdpc ${GDPC_SOURCE_FILES})

# Set compiler options
if(DEBUG)
    target_compile_options(gdpc PRIVATE "-g" "-O0" "-std=c99" "-Wall" "-Wextra" "-Wpedantic" "-Werror")
else()
    target_compile_options(gdpc PRIVATE "-O3" "-std=c99")
endif()
