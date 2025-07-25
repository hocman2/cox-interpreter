cmake_minimum_required(VERSION 3.13)

project(cox-interpreter)

file(GLOB_RECURSE SOURCE_FILES src/*.c src/*.h)

set(CMAKE_C_STANDARD 23)

add_executable(interpreter ${SOURCE_FILES})
