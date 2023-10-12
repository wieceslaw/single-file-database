#include <stdio.h>
#include "allocator/allocator.h"
#include "database/database.h"
#include "database/query/scheme_builder.h"
#include "database/query/join_builder.h"

void print_row_value(table_scheme *scheme, row_value value) {
    for (size_t i = 0; i < scheme->size; i++) {
        table_scheme_column column = scheme->columns[i];
        switch (column.type) {
            case COLUMN_TYPE_INT:
                printf("%d ", value->values[i].val_int);
                break;
            case COLUMN_TYPE_FLOAT:
                printf("%f ", value->values[i].val_float);
                break;
            case COLUMN_TYPE_STRING:
                printf("%s ", value->values[i].val_string);
                break;
            case COLUMN_TYPE_BOOL:
                printf("%d ", value->values[i].val_bool);
                break;
        }
    }
    printf(" \n");
}

void print_view(result_view_t view) {
    table_scheme *scheme = result_view_scheme(view);
    while (!result_view_is_empty(view)) {
        row_value value = result_view_get(view);
        print_row_value(scheme, value);
        row_value_free(scheme, value);
        result_view_next(view);
    }
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

//    column_value id = {.val_int = 2};
//    column_value name = {.val_string = "Adam"};
//    column_value age = {.val_int = 20};
//    column_value columns[] = {id, name, age};
//    struct row_value row = {(column_value *) &columns};
//    database_insert(db, "user", &row);

    query_t delete_query = {
            .table = "user",
            .where = where_condition_compare(
                    COMPARE_EQ,
                    operand_column("user", "id"),
                    operand_literal_int(2)
            ),
            .joins = NULL
    };
    database_delete(db, delete_query);

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
