cmake_minimum_required(VERSION 3.17)
project(ex4 C)

set(CMAKE_C_STANDARD 99)

add_executable(ex4 vector.h hash_funcs.h hashmap.h pair.c pair.h
        pair_char_int.h vector.c hashmap.c test_suite.c test_suite.h)