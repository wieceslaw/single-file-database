//
// Created by vyach on 16.10.2023.
//

#include <stdio.h>
#include <time.h>
#include "allocator/allocator.h"
#include "database/database.h"
#include "database/query/scheme_builder.h"

static void insert(database_t db, int n) {
    batch_builder_t batch = batch_builder_init(n);
    for (int i = 0; i < n; i++) {
        int integer = 228;
        float floating = 0.5f;
        int boolean = 1;
        char string[64] = "12345678123456781234567812345678";
        row_builder_t row = row_builder_init(4);
        row_builder_add(&row, column_int(integer));
        row_builder_add(&row, column_string(string));
        row_builder_add(&row, column_bool(!!boolean));
        row_builder_add(&row, column_float(floating));
        batch_builder_add(&batch, row);
    }
    database_insert(db, "test", batch);
    batch_builder_free(&batch);
}

static void create_table(database_t db) {
    scheme_builder_t scheme_builder = scheme_builder_init("test");
    scheme_builder_add_column(scheme_builder, "int", COLUMN_TYPE_INT);
    scheme_builder_add_column(scheme_builder, "string", COLUMN_TYPE_STRING);
    scheme_builder_add_column(scheme_builder, "bool", COLUMN_TYPE_BOOL);
    scheme_builder_add_column(scheme_builder, "float", COLUMN_TYPE_FLOAT);
    database_create_table(db, scheme_builder);
    scheme_builder_free(&scheme_builder);
}

int main() {
    file_open_mode mode = FILE_OPEN_CLEAR;
    file_settings settings = {.path = "C:\\Users\\vyach\\CLionProjects\\llp-lab1\\test.bin", .open_mode = mode};
    database_t db = database_init(&settings);
    if (mode == FILE_OPEN_CLEAR) {
        create_table(db);
    }
    insert(db, 25000);
    database_free(db);
    return 0;
}
