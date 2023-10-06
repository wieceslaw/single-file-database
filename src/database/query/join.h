
//
// Created by vyach on 06.10.2023.
//

#ifndef LLP_LAB1_JOIN_H
#define LLP_LAB1_JOIN_H

#include <stdbool.h>
#include "row_set.h"

typedef enum {
    JOIN_TYPE_INNER = 0,
    JOIN_TYPE_LEFT = 1,
    JOIN_TYPE_RIGHT = 2,
} join_type;

typedef struct join_table {
    char* table;
    char* alias;
} join_table;

typedef struct join_column {
    char *alias;
    char* column;
} join_column;

typedef struct join_condition {
    join_column first;
    join_column second;
} join_condition;

typedef struct join_info {
    join_table left;
    join_table right;
    join_type type;
    join_condition condition;
} join_info;

bool join_condition_check(join_info info, row_set_t row);

#endif //LLP_LAB1_JOIN_H
