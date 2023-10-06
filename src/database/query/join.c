//
// Created by vyach on 06.10.2023.
//

#include <assert.h>
#include "join.h"
#include "database/table/row.h"

bool join_condition_check(join_info info, row_set_t set) {
    char* first_table = info.condition.first.alias;
    char* second_table = info.condition.second.alias;
    table_row_t row1 = MAP_GET(set, first_table);
    table_row_t row2 = MAP_GET(set, second_table);
    assert(row1 != NULL && row2 != NULL);
    char* first_column_name = info.condition.first.column;
    char* second_column_name = info.condition.second.column;
    /* TODO: Implement
    column first_column = table_row_extract_col(row1, first_column_name);
    column second_column = table_row_extract_col(row2, second_column_name);
    assert(first_column.type == second_column.type);
    return column_compare(first_column, second_column);
    */
    return true;
}
