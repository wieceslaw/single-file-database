//
// Created by vyach on 13.10.2023.
//

#include <stdio.h>
#include <time.h>
#include "database/database.h"
#include "database/query/scheme_builder.h"

void print_row(row_t row) {
    for (size_t i = 0; i < row.size; i++) {
        column_t column = row.columns[i];
        switch (column.type) {
            case COLUMN_TYPE_INT:
                printf("%d ", column.value.val_int);
                break;
            case COLUMN_TYPE_FLOAT:
                printf("%f ", column.value.val_float);
                break;
            case COLUMN_TYPE_STRING:
                printf("%s ", column.value.val_string);
                break;
            case COLUMN_TYPE_BOOL:
                printf("%d ", column.value.val_bool);
                break;
        }
    }
    printf(" \n");
}

void print_view(result_view_t view) {
    int count = 0;
    while (!result_view_is_empty(view)) {
        row_t row = result_view_get(view);
        print_row(row);
        row_free(row);
        result_view_next(view);
        count++;
    }
    printf("count: %d \n", count);
}

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

void select(database_t db) {
    query_t query = {.table = "user", .where = NULL, .joins = NULL};
    selector_builder selector = selector_builder_init();
    selector_builder_add(selector, "user", "id");
    selector_builder_add(selector, "user", "name");
    selector_builder_add(selector, "user", "age");
    int count = 0;
    result_view_t view = database_select(db, query, selector);
    if (NULL == view) {
        printf("Can't create select \n");
        return;
    }
    while (!result_view_is_empty(view)) {
        row_t row = result_view_get(view);
        // print
        row_free(row);
        result_view_next(view);
        count++;
    }
    result_view_free(&view);
    selector_builder_free(&selector);
    printf("selected count: %d \n", count);
}

int main(void) {
    file_settings settings = {.path = "test.bin", .open_type = FILE_OPEN_CLEAR};
    database_t db = database_init(&settings);
    create_table(db);
    insert(db, 100000);
    clock_t begin = clock();
    select(db);
    clock_t end = clock();
    double time_spent = (double) (end - begin) / CLOCKS_PER_SEC;
    printf("time: %f \n", time_spent);
    database_free(db);
    return 0;
}