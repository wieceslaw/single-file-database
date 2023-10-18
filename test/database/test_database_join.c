//
// Created by vyach on 18.10.2023.
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

void selecting(database_t db) {
    join_builder_t join_builder = join_builder_init();
    join_builder_add(join_builder,
                     table_column_of(
                            "test",
                            "int"
                     ),
                     table_column_of(
                             "test2",
                             "int"
                     )
    );
    query_t query = {
            .table = "test",
            .where = NULL,
            .joins = join_builder
    };
    selector_builder selector = selector_builder_init();
    selector_builder_add(selector, "test", "int");
    selector_builder_add(selector, "test", "string");
    selector_builder_add(selector, "test", "bool");
    selector_builder_add(selector, "test", "float");
    selector_builder_add(selector, "test2", "int");
    selector_builder_add(selector, "test2", "string");
    selector_builder_add(selector, "test2", "bool");
    selector_builder_add(selector, "test2", "float");

    clock_t begin = clock();

    result_view_t view = database_select(db, query, selector);
    if (NULL == view) {
        printf("Can't create select \n");
        return;
    }
    while (!result_view_is_empty(view)) {
        row_t row = result_view_get(view);
        print_row(row);
        row_free(row);
        result_view_next(view);
    }
    result_view_free(&view);
    clock_t end = clock();
    double time_spent = (double) (end - begin) / CLOCKS_PER_SEC;
    printf("%f", time_spent);
    selector_builder_free(&selector);
    join_builder_free(&join_builder);
}

int main(void) {
    file_settings settings = {.path = "C:\\Users\\vyach\\CLionProjects\\llp-lab1\\test.bin", .open_mode = FILE_OPEN_EXIST};
    database_t db = database_init(&settings);
    selecting(db);
    database_free(db);
    return 0;
}
