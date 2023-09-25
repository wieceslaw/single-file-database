cmake_minimum_required(VERSION 3.16.3)
project(llp_lab1 C)

set(CMAKE_C_STANDARD 11)

include_directories(include/)

add_executable(llp_lab1
        main.c
        src/heap/heap.h
        src/allocator/buffer.h
        src/heap/list.h
        src/allocator/allocator.h
        src/heap/list.c
        src/heap/list_iterator.c
        src/allocator/impl/allocator.c
        include/pool.h
        include/table_iterator.h
        include/table.h
        include/query/cursor.h
        src/allocator/impl/blocklist.c
        src/allocator/impl/blocklist.h
)
