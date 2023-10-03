//
// Created by vyach on 03.10.2023.
//
#include <malloc.h>
#include "query.h"

query_info *FROM(char *table_name, char *alias) {
    query_info *q = malloc(sizeof(query_info));
    *q = (query_info) {
            .type = QUERY_INFO_FROM,
            .from = {.table_name = "1", .alias="2"}
    };
    return q;
}

join_column *COL(const char *table_alias, const char *column_name) {
    join_column *column = malloc(sizeof(join_column));
    *column = (join_column) {.table_alias = table_alias, .column_name = column_name};
    return column;
}

join_condition *ON(join_column *first, join_column *second) {
    join_condition *condition = malloc(sizeof(join_condition));
    *condition = (join_condition) {.first = first, .second = second};
    return condition;
}

query_info *JOIN(query_info *first, query_info *second, join_type type, join_condition *condition) {
    query_info *q = malloc(sizeof(query_info));
    *q = (query_info) {
            .type = QUERY_INFO_JOIN,
            .join = {.first = first, .second = second, .condition = condition, .type = type}
    };
    return q;
}

query_info *WHERE(query_info *base) {
    query_info *q = malloc(sizeof(query_info));
    *q = (query_info) {
            .type = QUERY_INFO_WHERE,
            .where = {.base = base}
    };
    return q;
}

void query_info_free(query_info *);
