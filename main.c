#include <stdio.h>
#include <string.h>
#include "src/allocator/allocator.h"

int main(int argc, char *argv[]) {
    file_settings settings = {.path = "test.bin", .open_type = FILE_OPEN_CLEAR};
    allocator_t *allocator;
    int res = allocator_init(&settings, &allocator);
    if (res != FILE_ST_OK) {
        printf("File error %d", res);
        return -1;
    }

    if (allocator_reserve_pages(allocator, 100) != ALLOCATOR_SUCCESS) {
        printf("unable extend");
        return -1;
    }
    page_t* ref = allocator_get_page(allocator);
    if (ref == NULL) {
        printf("unable to get block");
        return -1;
    }
    void *ptr = page_ptr(ref);
    strcpy(ptr, "1234567890");
    allocator_unmap_page(allocator, ref);

    allocator_free(allocator);
    return 0;
}
