cmake_minimum_required(VERSION 3.19)
project(Ex2)

set(CMAKE_CXX_STANDARD 14)

include_directories(.)

add_executable(Ex2
        main.cpp
        uthreads.cpp
        uthreads.h)
