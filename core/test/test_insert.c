//
// Created by vyach on 13.10.2023.
//

#include <stdio.h>
#include <assert.h>
#include <time.h>
#include "allocator/allocator.h"
#include "database/database.h"
#include "database/query/scheme_builder.h"

void insert(database_t db, int n, char* table_name) {
    batch_builder_t batch = batch_builder_init(n);
    for (int i = 0; i < n; i++) {
        int integer;
        float floating;
        int boolean;
        char string[64];
        scanf("%d", &integer);
        scanf("%s", string);
        scanf("%d", &boolean);
        scanf("%f", &floating);
        row_builder_t row = row_builder_init(4);
        row_builder_add(&row, column_int(integer));
        row_builder_add(&row, column_string(string));
        row_builder_add(&row, column_bool(!!boolean));
        row_builder_add(&row, column_float(floating));
        batch_builder_add(&batch, row);
    }

    clock_t begin = clock();
    database_insert(db, table_name, batch);
    clock_t end = clock();
    double time_spent = (double)(end - begin) / CLOCKS_PER_SEC;
    printf("%f", time_spent);

    batch_builder_free(&batch);
}

void create_table(database_t db, char* table_name) {
    scheme_builder_t scheme_builder = scheme_builder_init(table_name);
    scheme_builder_add_column(scheme_builder, "int", COLUMN_TYPE_INT);
    scheme_builder_add_column(scheme_builder, "string", COLUMN_TYPE_STRING);
    scheme_builder_add_column(scheme_builder, "bool", COLUMN_TYPE_BOOL);
    scheme_builder_add_column(scheme_builder, "float", COLUMN_TYPE_FLOAT);
    database_create_table(db, scheme_builder);
    scheme_builder_free(&scheme_builder);
}

int main(int argc, char *argv[]) {
    assert(argc == 4);
    int n = atoi(argv[1]);
    file_open_mode mode = atoi(argv[2]);
    char* table_name = argv[3];
    file_settings settings = {.path = "C:\\Users\\vyach\\CLionProjects\\llp-lab1\\test.bin", .open_mode = mode};
    database_t db = database_init(&settings);
    if (mode == FILE_OPEN_CLEAR) {
        create_table(db, table_name);
    }

    insert(db, n, table_name);

    database_free(db);
    return 0;
}
