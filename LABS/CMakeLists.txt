cmake_minimum_required(VERSION 3.6)
project(LABS)

set(CMAKE_C_STANDARD 99)

set(SOURCE_FILES LAB1/omp1.c)
add_executable(LABS ${SOURCE_FILES})