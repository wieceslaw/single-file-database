cmake_minimum_required(VERSION 3.16.3)
project(llp_lab1 C)

set(CMAKE_C_STANDARD 11)

include_directories(src/)

add_executable(llp_lab1
        main.c
        src/heap/heap.h
        src/buffer/buffer.h
        src/heap/private/pagelist.h
        src/allocator/allocator.h
        src/heap/private/pagelist.c
        src/allocator/private/allocator.c
        src/database/table/table.h
        src/allocator/private/blocklist.c
        src/allocator/private/blocklist.h
        src/pool/pool.h
        src/heap/private/heap.c
        src/pool/pool.c
        src/buffer/buffer.c
        test/test_heap.c
        test/test_pool.c
        src/database/table/table.c
        test/test.h
        src/database/database.c
        src/database/database.h
        src/database/cursor/cursor.h
        src/database/cursor/cursor.c
        src/database/query/where_condition.h
        src/util/vector/vector.h
        src/util/vector/vector.c
        src/util/defines.h
        src/util/map/map.h
        src/util/map/map.c
        src/util/list/list.h
        src/util/list/list.c
        src/util/map/map_impl.h
        src/util/map/map_impl.c
        src/util/exceptions/exceptions.c
        src/util/exceptions/exceptions.h
        test/test_map.c
        src/database/query/join_builder.h
        src/database/query/join_builder.c
        src/database/query/where_condition.c
        src/util/map/set.h
        src/util/string.c
        src/util/string.h
        src/database/query/updater_builder.h
        src/database/query/updater_builder.c
        src/database/query/scheme_builder.h
        src/database/query/scheme_builder.c
        src/database/query/selector_builder.h
        src/database/query/selector_builder.c
        src/database/query/result_view.h
        src/database/query/result_view.c
)
