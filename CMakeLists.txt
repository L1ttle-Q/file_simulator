cmake_minimum_required(VERSION 3.10)
project(file_simulator)

set(CMAKE_CXX_STANDARD 17)

include_directories(include)

add_executable(file_simulator
    src/file_simulator.cpp
    src/file_save.cpp
)