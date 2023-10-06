#include <stdio.h>
#include "allocator/allocator.h"
#include "database/database.h"

int main(void) {
    file_settings settings = {.path = "test.bin", .open_type = FILE_OPEN_EXIST};
    database_t db = database_init(&settings);
    printf("created db");
    database_free(db);
    return 0;
}
