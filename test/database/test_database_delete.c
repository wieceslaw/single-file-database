//
// Created by vyach on 13.10.2023.
//

#include "database/database.h"

void delete(database_t db) {
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
}
