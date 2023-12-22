//
// Created by vyach on 03.10.2023.
//

#ifndef LLP_LAB1_WHERE_CONDITION_H
#define LLP_LAB1_WHERE_CONDITION_H

#include "defines.h"
#include "exceptions/exceptions.h"
#include "database/table/table.h"

typedef enum {
    CONDITION_AND = 0,
    CONDITION_OR = 1,
    CONDITION_NOT = 2,
    CONDITION_COMPARE = 3,
} condition_type;

typedef enum {
    COMPARE_EQ = 0,
    COMPARE_NE = 1,
    COMPARE_GT = 2,
    COMPARE_LT = 3,
    COMPARE_GE = 4,
    COMPARE_LE = 5,
} comparing_type;

typedef enum {
    OPERAND_VALUE_LITERAL = 0,
    OPERAND_VALUE_COLUMN = 1,
} operand_value_type;

typedef struct {
    operand_value_type type;
    union {
        column_t literal;
        column_description column;
    };
} operand;

typedef struct where_condition {
    condition_type type;
    union {
        struct {
            struct where_condition *first;
            struct where_condition *second;
        } and;
        struct {
            struct where_condition *first;
            struct where_condition *second;
        } or;
        struct {
            struct where_condition *first;
        } not;
        struct {
            comparing_type type;
            operand first;
            operand second;
        } compare;
    };
} where_condition;

void where_condition_free(where_condition *condition);

operand operand_column(char *table, char *column);

operand operand_literal_float(float value);

operand operand_literal_int(int32_t value);

operand operand_literal_bool(bool value);

/// THROWS: [MALLOC_EXCEPTION]
operand operand_literal_string(char *value);

/// THROWS: [MALLOC_EXCEPTION]
where_condition *where_condition_compare(comparing_type type, operand op1, operand op2);

/// THROWS: [MALLOC_EXCEPTION]
where_condition *where_condition_and(where_condition *first, where_condition *second);

/// THROWS: [MALLOC_EXCEPTION]
where_condition *where_condition_or(where_condition *first, where_condition *second);

/// THROWS: [MALLOC_EXCEPTION]
where_condition *where_condition_not(where_condition *first);

bool columns_equals(column_t first, column_t second);

#endif //LLP_LAB1_WHERE_CONDITION_H
