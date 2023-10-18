//
// Created by vyach on 16.10.2023.
//

#include <stdio.h>
#include <time.h>
#include "allocator/allocator.h"
#include "database/database.h"
#include "database/query/scheme_builder.h"

static void insert(database_t db, int n, char* table_name) {
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
    database_insert(db, table_name, batch);
    batch_builder_free(&batch);
}

static void create_table(database_t db, char* table_name) {
    scheme_builder_t scheme_builder = scheme_builder_init(table_name);
    scheme_builder_add_column(scheme_builder, "int", COLUMN_TYPE_INT);
    scheme_builder_add_column(scheme_builder, "string", COLUMN_TYPE_STRING);
    scheme_builder_add_column(scheme_builder, "bool", COLUMN_TYPE_BOOL);
    scheme_builder_add_column(scheme_builder, "float", COLUMN_TYPE_FLOAT);
    database_create_table(db, scheme_builder);
    scheme_builder_free(&scheme_builder);
}

int main() {
    file_open_mode mode = FILE_OPEN_EXIST;
    char* table_name = "test2";
    file_settings settings = {.path = "C:\\Users\\vyach\\CLionProjects\\llp-lab1\\test.bin", .open_mode = mode};
    database_t db = database_init(&settings);
    create_table(db, table_name);
    if (mode == FILE_OPEN_CLEAR) {
        create_table(db, table_name);
    }
    insert(db, 1000, table_name);
    database_free(db);
    return 0;
}
