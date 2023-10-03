//
// Created by vyach on 03.10.2023.
//

#ifndef LLP_LAB1_QUERY_H
#define LLP_LAB1_QUERY_H

#include <stdint-gcc.h>

typedef enum {
    JOIN_TYPE_INNER = 0,
    JOIN_TYPE_LEFT = 1,
    JOIN_TYPE_RIGHT = 2,
} join_type;

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

typedef enum {
    TYPE_INT = 0,
    TYPE_FLOAT = 1,
    TYPE_STRING = 2,
    TYPE_BOOL = 3
} column_type;

typedef struct {
    const char *table_alias;
    const char *column_name;
} column_alias;

typedef struct {
    column_alias *first, *second;
} join_condition;

typedef union {
    float val_float;
    int32_t val_int;
    uint8_t val_bool;
    char *val_string;
} column_value;

typedef struct {
    operand_type type;
    union {
        struct {
            column_type type;
            column_value value;
        } literal;
        column_alias column;
    };
} operand;

typedef struct where_condition where_condition;

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

typedef struct query_info query_info;

struct query_info {
    query_info_type type;
    union {
        struct {
            const char *table_name;
            const char *alias;
        } from;
        struct {
            query_info *first;
            query_info *second;
            join_type type;
            join_condition *condition;
        } join;
        struct {
            query_info *base;
            where_condition *condition;
        } where;
    };
};

void query_info_free(query_info *);

query_info *FROM(char *table_name, char *alias);

column_alias *COL(const char *column_name, const char *table_alias);

join_condition *ON(column_alias *first, column_alias *second);

query_info *JOIN(query_info *first, query_info *second, join_type type, join_condition *condition);

query_info *WHERE(query_info *base, where_condition* condition);

#endif //LLP_LAB1_QUERY_H
