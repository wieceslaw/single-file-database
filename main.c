#include <stdio.h>
#include "src/allocator/allocator.h"
#include "pool/pool.h"
#include "table/table.h"
#include "test/test.h"


int main(void) {
//    column_t columns[4] = {
//            {.name = "id", .type = COL_INT},
//            {.name = "name", .type = COL_STRING},
//            {.name = "age", .type = COL_INT}
//    };
//    schema_t schema = {.size = 4, .columns = columns};
//    table_t table = {.name = "user", .schema = &schema};

    file_settings settings = {.path = "test.bin", .open_type = FILE_OPEN_CLEAR};
    allocator_t *allocator;
    int res = allocator_init(&settings, &allocator);
    if (res != FILE_ST_OK) {
        printf("File error %d", res);
        return -1;
    }

    test_heap(allocator);

//    if (settings.open_type == FILE_OPEN_CLEAR || settings.open_type == FILE_OPEN_CREATE) {
//        offset_t offset = pool_create(allocator);
//        if (0 == offset) {
//            return -1;
//        }
//        allocator_set_entrypoint(allocator, offset);
//    }
//
//    pool_t *pool = pool_init(allocator, allocator_get_entrypoint(allocator));
//    if (NULL == pool) {
//        return -1;
//    }

    if (allocator_free(allocator) != FILE_ST_OK) {
        printf("unable free allocator");
        return -1;
    }
    return 0;
}
