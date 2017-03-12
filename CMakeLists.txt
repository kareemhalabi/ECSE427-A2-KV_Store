cmake_minimum_required(VERSION 3.6)
project(a2_testcase)

set(CMAKE_CXX_STANDARD 11)

set(SOURCE_FILES1
        comp310_a2_test.h
        comp310_a2_test1.c
        a2_lib.h)

set(SOURCE_FILES2
        comp310_a2_test.h
        comp310_a2_test2.c
        a2_lib.h)

set(SOURCE_FILES3
        comp310_simple_test.c
        a2_lib.h)

add_executable(a2_testcase1 ${SOURCE_FILES1})
add_executable(a2_testcase2 ${SOURCE_FILES2})
add_executable(a2_simplecase ${SOURCE_FILES3})