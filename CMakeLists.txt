cmake_minimum_required(VERSION 3.16.3)
project(single_file_database C)

set(CMAKE_C_STANDARD 11)

add_subdirectory(util/)
add_subdirectory(core/)
add_subdirectory(parser/)
add_subdirectory(network/)
add_subdirectory(server/)
add_subdirectory(client/)
