//
// Created by vyach on 13.10.2023.
//

#include <stdio.h>
#include <time.h>
#include "database/database.h"

void delete(database_t db) {
    query_t query = {
            .table = "test",
            .where = where_condition_compare(
                    COMPARE_EQ,
                    operand_column("test", "bool"),
                    operand_literal_bool(true)
            ),
            .joins = NULL
    };

    clock_t begin = clock();
    database_delete(db, query);
    clock_t end = clock();
    double time_spent = (double)(end - begin) / CLOCKS_PER_SEC;
    printf("%f", time_spent);
}

int main() {
    file_settings settings = {.path = "C:\\Users\\vyach\\CLionProjects\\llp-lab1\\test.bin", .open_mode = FILE_OPEN_EXIST};
    database_t db = database_init(&settings);
    delete(db);
    database_free(db);
    return 0;
}
