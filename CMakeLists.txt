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
        src/database/query/query.h
        src/database/query/query.c
        src/database/table/row.h
        src/database/table/row.c
        src/util/vector/vector.h
        src/util/vector/vector.c
        src/util/defines.h
        src/util/map/map.h
        src/util/map/map.c
        src/util/list/list.h
        src/util/list/list.c
        src/util/map/map_impl.h
        src/util/map/map_impl.c
)

if (UNIX)
target_compile_options(llp_lab1 PRIVATE
        -Werror -Wall -Wextra -pedantic -Wmissing-prototypes -Wstrict-prototypes
        -Wno-unused-function -Wno-unused-but-set-variable
#        -fsanitize=address -fsanitize=pointer-compare -fsanitize=pointer-subtract -fsanitize=leak
#        -fsanitize=undefined -fsanitize-address-use-after-scope
)
endif(UNIX)
#target_link_directories(llp_lab1 PRIVATE "C:/Program Files/Microsoft Visual Studio/2022/Professional/VC/Tools/Llvm/x64/lib/clang/16/lib/windows")
#target_link_libraries(llp_lab1 PRIVATE clang_rt.asan_dynamic-x86_64 clang_rt.asan_dynamic_runtime_thunk-x86_64)
