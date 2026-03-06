cmake_minimum_required(VERSION 3.20.0)

add_library(posix_with_serial STATIC serial.c interface.c)
target_include_directories(posix_with_serial PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/include)