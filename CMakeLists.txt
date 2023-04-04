cmake_minimum_required(VERSION 3.24)
project(SimpleCalculator C)

set(CMAKE_C_STANDARD 99)

add_executable(SimpleCalculator main.c instruction.c instruction.h string_to_instruction.c string_to_instruction.h)
