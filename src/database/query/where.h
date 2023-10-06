//
// Created by vyach on 03.10.2023.
//

#ifndef LLP_LAB1_WHERE_H
#define LLP_LAB1_WHERE_H
#include "util/defines.h"
#include "row_set.h"

typedef enum {
    QUERY_INFO_FROM = 0,
    QUERY_INFO_JOIN = 1,
    QUERY_INFO_WHERE = 2,
} query_info_type;

typedef enum {
    WHERE_COND_AND = 0,
    WHERE_COND_OR = 1,
    WHERE_COND_NOT = 2,
    WHERE_COND_COMPARE = 3,
} where_condition_type;

typedef enum {
    COMPARE_IS = 0,
    COMPARE_EQ = 1,
    COMPARE_NE = 2,
    COMPARE_GT = 3,
    COMPARE_LT = 4,
    COMPARE_GE = 5,
    COMPARE_LE = 6,
} comparing_type;

typedef enum {
    OPERAND_LIT = 0,
    OPERAND_COL = 1,
} operand_type;

typedef struct {
    const char *table_alias;
    const char *column_name;
} column_alias;

typedef struct {
    operand_type type;
    union {
        struct {
            int a;
//            column_type type;
//            column_value value;
        } literal;
        column_alias column;
    };
} operand;

typedef struct where_condition where_condition;

bool where_condition_check(where_condition condition, row_set_t set);

struct where_condition {
    where_condition_type condition_type;
    union {
        struct {
            where_condition *first;
            where_condition *second;
        } and;
        struct {
            where_condition *first;
            where_condition *second;
        } or;
        struct {
            where_condition *first;
        } not;
        struct {
            comparing_type type;
            operand *op1;
            operand *op2;
        } compare;
    };
    // (P) OR (P)
    // (P) AND (P)
    // NOT (P)
    // (L|C) & (L|C)
    // int, float: ==, !=, <, >, >=, <=
    // bool: ==, !=
    // string: ==, !=
};

#endif //LLP_LAB1_WHERE_H
