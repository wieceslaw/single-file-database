//
// Created by vyach on 12.10.2023.
//

#include <stdio.h>
#include <time.h>
#include "database/database.h"
#include "database/query/scheme_builder.h"
#include "database/query/row_batch.h"

void test_insert(database_t db, int n) {
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

void test_select(database_t db, char* table_name) {
    query_t query = {.table = table_name, .where = NULL, .joins = NULL};
    selector_builder selector = selector_builder_init();
    selector_builder_add(selector, "user", "id");
    selector_builder_add(selector, "user", "name");
    selector_builder_add(selector, "user", "age");
    result_view_t view = database_select(db, query, selector);
    while (!result_view_is_empty(view)) {
        row_t row = result_view_get(view);
        row_free(row);
        result_view_next(view);
    }
    result_view_free(&view);
    selector_builder_free(&selector);
}
