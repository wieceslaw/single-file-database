//
// Created by vyach on 13.10.2023.
//

#include "database/database.h"

void update(database_t db) {
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
}
