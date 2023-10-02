cmake_minimum_required(VERSION 3.16.3)
project(llp_lab1 C)

set(CMAKE_C_STANDARD 11)

include_directories(src/)

add_executable(llp_lab1
        main.c
        src/heap/heap.h
        src/buffer/buffer.h
        src/heap/impl/list.h
        src/allocator/allocator.h
        src/heap/impl/list.c
        src/allocator/impl/allocator.c
        src/database/table/table.h
        src/allocator/impl/blocklist.c
        src/allocator/impl/blocklist.h
        src/pool/pool.h
        src/heap/impl/heap.c
        src/pool/pool.c
        src/buffer/buffer.c
        test/test_heap.c
        test/test_pool.c
        src/database/table/table.c
        test/test.h
        src/database/database.c
        src/database/database.h
        src/database/table/impl/cursor.h
        src/database/table/impl/cursor.c
        src/database/table/impl/joinset.h
        src/database/table/impl/joinset.c
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
