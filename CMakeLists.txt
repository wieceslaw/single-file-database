cmake_minimum_required(VERSION 3.16.3)
project(llp_lab1 C)

set(CMAKE_C_STANDARD 11)

include_directories(include/)

add_executable(llp_lab1
        main.c
        include/heap/heap.h
        include/heap/buffer.h
        include/heap/list.h
        include/allocator/allocator.h
        src/heap/list.c
        src/heap/list_iterator.c
        src/allocator/allocator.c
        include/pool.h
        include/table_iterator.h
        src/heap/heap.c
        src/pool.c
        include/table.h
        include/query/cursor.h
)
