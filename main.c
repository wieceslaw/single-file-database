#include <stdio.h>
#include "allocator/allocator.h"
#include "database/database.h"

int main(void) {
    file_settings settings = {.path = "test.bin", .open_type = FILE_OPEN_EXIST};
    database_t *db = database_init(&settings);
    if (NULL == db) {
        printf("unable to init database");
        return -1;
    }
    if (database_free(db) != DATABASE_OP_OK) {
        printf("unable to free database");
        return -1;
    }
    return 0;
}
