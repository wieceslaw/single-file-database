//
// Created by vyach on 13.10.2023.
//

#include <time.h>
#include <stdio.h>
#include "database/database.h"

void update(database_t db) {
    where_condition *condition = where_condition_compare(
            COMPARE_EQ,
            operand_column("test", "bool"),
            operand_literal_bool(true)
    );
    query_t query = {
            .table = "test",
            .where = condition,
            .joins = NULL
    };

    updater_builder_t updater = updater_builder_init();
    updater_builder_add(updater,
                        column_updater_of(
                                "bool",
                                column_bool(true)
                        )
    );
    updater_builder_add(updater,
                        column_updater_of(
                                "string",
                                column_string("updated")
                        )
    );

    clock_t begin = clock();
    database_update(db, query, updater);
    clock_t end = clock();
    double time_spent = (double) (end - begin) / CLOCKS_PER_SEC;
    printf("%f", time_spent);
    where_condition_free(condition);
    updater_builder_free(&updater);
}

int main() {
    file_settings settings = {.path = "C:\\Users\\vyach\\CLionProjects\\llp-lab1\\test.bin", .open_mode = FILE_OPEN_EXIST};
    database_t db = database_init(&settings);
    update(db);
    database_free(db);
    return 0;
}
