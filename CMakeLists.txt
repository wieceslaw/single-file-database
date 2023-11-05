cmake_minimum_required(VERSION 3.16.3)
project(single_file_database C)

set(CMAKE_C_STANDARD 11)

add_subdirectory(core/)
add_subdirectory(parser/)
