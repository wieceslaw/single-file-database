//
// Created by vyach on 13.10.2023.
//

#include <time.h>
#include <stdio.h>
#include "database/Database.h"

static void update(Database db) {
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
                                ColumnOfBool(true)
                        )
    );
    updater_builder_add(updater,
                        column_updater_of(
                                "string",
                                ColumnOfString("updated")
                        )
    );

    clock_t begin = clock();
    DatabaseUpdateQuery(db, query, updater);
    clock_t end = clock();
    double time_spent = (double) (end - begin) / CLOCKS_PER_SEC;
    printf("%f", time_spent);
    where_condition_free(condition);
    updater_builder_free(&updater);
}

int main(void) {
    file_settings settings = {.path = "C:\\Users\\vyach\\CLionProjects\\llp-lab1\\test.bin", .open_mode = FILE_OPEN_EXIST};
    Database db = DatabaseNew(&settings);
    update(db);
    DatabaseFree(db);
    return 0;
}
