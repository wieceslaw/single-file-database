cmake_minimum_required(VERSION 3.16.3)
project(single_file_database C)

set(CMAKE_C_STANDARD 11)

# [Compile options]
add_compile_options(
        -Werror
        -Wall
        -Wextra
        -pedantic
        -Wmissing-prototypes
        -Wstrict-prototypes
)
add_compile_options(
        -Wno-unused-function
        -Wno-unused-but-set-variable
)

# [Sanitizers]
set(SANITIZERS
        -fsanitize=address
        -fsanitize=pointer-compare
        -fsanitize=pointer-subtract
        -fsanitize=leak
        -fsanitize=undefined
        -fsanitize-address-use-after-scope
)
add_compile_options(${SANITIZERS})
add_link_options(${SANITIZERS})

add_subdirectory(util/)
add_subdirectory(core/)
add_subdirectory(parser/)
add_subdirectory(network/)
add_subdirectory(server/)
add_subdirectory(client/)
