cmake_minimum_required(VERSION 3.15)
project(HW01)

set(CMAKE_CXX_STANDARD 14)

include_directories(.)

add_executable(HW01
        Commands.cpp
        Commands.h
        signals.cpp
        signals.h
        smash.cpp)
