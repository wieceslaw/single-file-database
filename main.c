#include <stdio.h>
#include <time.h>
#include "allocator/allocator.h"
#include "database/database.h"
#include "database/query/scheme_builder.h"
#include "database/query/join_builder.h"
#include "test/test.h"

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

int main(void) {
    file_settings settings = {.path = "test.bin", .open_type = FILE_OPEN_EXIST};
    database_t db = database_init(&settings);

//    scheme_builder_t scheme_builder = scheme_builder_init("user");
//    scheme_builder_add_column(scheme_builder, "id", COLUMN_TYPE_INT);
//    scheme_builder_add_column(scheme_builder, "name", COLUMN_TYPE_STRING);
//    scheme_builder_add_column(scheme_builder, "age", COLUMN_TYPE_INT);
//    database_create_table(db, scheme_builder_build(scheme_builder));
//    scheme_builder_free(&scheme_builder);

//    test_insert(db, 10000);

//    clock_t begin = clock();
//    test_select(db, "user");
//    clock_t end = clock();
//    double time_spent = (double) (end - begin) / CLOCKS_PER_SEC;
//    printf("time: %f \n", time_spent);

    test_delete(db, "user");

    query_t query = {.table = "user", .where = NULL, .joins = NULL};
    selector_builder selector = selector_builder_init();
    selector_builder_add(selector, "user", "id");
    selector_builder_add(selector, "user", "name");
    selector_builder_add(selector, "user", "age");
    result_view_t view = database_select(db, query, selector);
    print_view(view);
    result_view_free(&view);
    selector_builder_free(&selector);

    database_free(db);
    return 0;
}


int maind(void) {
    file_settings settings = {.path = "test.bin", .open_type = FILE_OPEN_EXIST};
    database_t db = database_init(&settings);

//    scheme_builder_t scheme_builder = scheme_builder_init("user");
//    scheme_builder_add_column(scheme_builder, "id", COLUMN_TYPE_INT);
//    scheme_builder_add_column(scheme_builder, "name", COLUMN_TYPE_STRING);
//    scheme_builder_add_column(scheme_builder, "age", COLUMN_TYPE_INT);
//    database_create_table(db, scheme_builder_build(scheme_builder));
//    scheme_builder_free(&scheme_builder);

//    column_value id = {.val_int = 2};
//    column_value name = {.val_string = "Adam"};
//    column_value age = {.val_int = 20};
//    column_value columns[] = {id, name, age};
//    struct row_value row = {(column_value *) &columns};
//    database_insert(db, "user", &row);

    query_t delete_query = {
            .table = "user",
            .where = NULL,
//            .where = where_condition_compare(
//                    COMPARE_EQ,
//                    operand_column("user", "id"),
//                    operand_literal_int(2)
//            ),
            .joins = NULL
    };
    database_delete(db, delete_query);

    query_t update_query = {
            .table = "user",
            .where = where_condition_compare(
                    COMPARE_EQ,
                    operand_column("user", "id"),
                    operand_literal_int(2)
            ),
            .joins = NULL
    };
    updater_builder_t updater = updater_builder_init();
    updater_builder_add(updater, column_updater_of("age", (column_t) {
            .type = COLUMN_TYPE_INT,
            .value = (column_value) {.val_int = 32}}
    ));
    database_update(db, update_query, updater);
    updater_builder_free(&updater);

    query_t query = {.table = "user", .where = NULL, .joins = NULL};
    selector_builder selector = selector_builder_init();
    selector_builder_add(selector, "user", "id");
    selector_builder_add(selector, "user", "name");
    selector_builder_add(selector, "user", "age");
    result_view_t view = database_select(db, query, selector);
    print_view(view);
    result_view_free(&view);
    selector_builder_free(&selector);

//    join_builder_t join_builder = join_builder_init();
//    join_builder_add(
//            join_builder,
//            table_column_of("subscription", "id"),
//            table_column_of("payment", "subscription_id")
//    );
//    join_builder_free(&join_builder);

    database_free(db);
    return 0;
}
