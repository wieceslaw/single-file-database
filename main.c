#include <stdio.h>
#include <strings.h>
#include "allocator/allocator.h"


int main(int argc, char *argv[]) {
    file_settings settings = {.path = "test.bin", .open_type = FILE_OPEN_EXIST};
    allocator *allocator;
    int res = allocator_init(&settings, &allocator);
    if (res != FILE_ST_OK) {
        printf("File error %d", res);
        return -1;
    }
//    allocator_reserve_blocks(allocator, 4);
    block* block = allocator_get_block(allocator);
    if (block == NULL) {
        printf("unable to get block");
        return -1;
    }
    void* ptr = block_ptr(block);
    strcpy(ptr, "1234567890");
    allocator_unmap_block(allocator, block);

    allocator_free(allocator);
    printf("Hello, World!\n");
    return 0;
}
