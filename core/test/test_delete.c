//
// Created by vyach on 13.10.2023.
//

#include <stdio.h>
#include <time.h>
#include "database/Database.h"
#include "query/where_condition.h"

static void delete(Database db) {
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

    clock_t begin = clock();
    DatabaseDeleteQuery(db, query);
    clock_t end = clock();
    double time_spent = (double)(end - begin) / CLOCKS_PER_SEC;
    where_condition_free(condition);
    printf("%f", time_spent);
}

int main(void) {
    file_settings settings = {.path = "C:\\Users\\vyach\\CLionProjects\\llp-lab1\\test.bin", .open_mode = FILE_OPEN_EXIST};
    Database db = DatabaseNew(&settings);
    delete(db);
    DatabaseFree(db);
    return 0;
}
