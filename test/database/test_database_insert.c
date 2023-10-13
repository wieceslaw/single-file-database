//
// Created by vyach on 13.10.2023.
//

#include <stdio.h>
#include <time.h>
#include "allocator/allocator.h"
#include "database/database.h"
#include "database/query/scheme_builder.h"

void insert(database_t db, int n) {
    batch_builder_t batch = batch_builder_init(n);
    for (int i = 0; i < n; i++) {
        char buffer[32];
        sprintf(buffer, "user_id:%d", i);
        row_builder_t row = row_builder_init(3);
        row_builder_add(&row, column_int(i));
        row_builder_add(&row, column_string(buffer));
        row_builder_add(&row, column_int(i * 2));
        batch_builder_add(&batch, row);
    }
    database_insert(db, "user", batch);
    batch_builder_free(&batch);
}

void create_table(database_t db) {
    scheme_builder_t scheme_builder = scheme_builder_init("user");
    scheme_builder_add_column(scheme_builder, "id", COLUMN_TYPE_INT);
    scheme_builder_add_column(scheme_builder, "name", COLUMN_TYPE_STRING);
    scheme_builder_add_column(scheme_builder, "age", COLUMN_TYPE_INT);
    database_create_table(db, scheme_builder_build(scheme_builder));
    scheme_builder_free(&scheme_builder);
}

int main() {
    file_settings settings = {.path = "test.bin", .open_type = FILE_OPEN_CLEAR};
    database_t db = database_init(&settings);
    create_table(db);
    clock_t begin = clock();
    insert(db, 10000);
    clock_t end = clock();
    double time_spent = (double) (end - begin) / CLOCKS_PER_SEC;
    printf("time: %f \n", time_spent);
    database_free(db);
    return 0;
}
