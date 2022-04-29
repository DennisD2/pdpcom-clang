cmake_minimum_required(VERSION 3.21)
project(terminal C)

set(CMAKE_C_STANDARD 99)

add_executable(terminal main.c tty.c tty.h)
