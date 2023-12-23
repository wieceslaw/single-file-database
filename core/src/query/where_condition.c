//
// Created by vyach on 06.10.2023.
//

#include <stdbool.h>
#include <assert.h>
#include <string.h>
#include "util_string.h"
#include "where_condition.h"

/// THROWS: [MALLOC_EXCEPTION]
operand operand_column(char *table, char *column) {
    return (operand) {
            .type = OPERAND_VALUE_COLUMN,
            .column = {
                    .type = COLUMN_DESC_NAME,
                    .name = {
                            .table_name = string_copy(table),
                            .column_name = string_copy(column)
                    }
            }
    };
}

operand operand_literal_float(float value) {
    return (operand) {
            .type = OPERAND_VALUE_LITERAL,
            .literal = {.type = COLUMN_TYPE_FLOAT, .value = {.f32 = value}}
    };
}

operand operand_literal_int(int32_t value) {
    return (operand) {
            .type = OPERAND_VALUE_LITERAL,
            .literal = {.type = COLUMN_TYPE_INT, .value = {.i32 = value}}
    };
}

operand operand_literal_bool(bool value) {
    return (operand) {
            .type = OPERAND_VALUE_LITERAL,
            .literal = {.type = COLUMN_TYPE_BOOL, .value = {.b8 = value}}
    };
}

/// THROWS: [MALLOC_EXCEPTION]
operand operand_literal_string(char *value) {
    return (operand) {
            .type = OPERAND_VALUE_LITERAL,
            .literal = {.type = COLUMN_TYPE_STRING, .value = {.str = string_copy(value)}}
    };
}

/// THROWS: [MALLOC_EXCEPTION]
where_condition *where_condition_compare(comparing_type type, operand op1, operand op2) {
    where_condition *result = rmalloc(sizeof(where_condition));
    result->type = CONDITION_COMPARE;
    result->compare.type = type;
    result->compare.first = op1;
    result->compare.second = op2;
    return result;
}

/// THROWS: [MALLOC_EXCEPTION]
where_condition *where_condition_and(where_condition *first, where_condition *second) {
    where_condition *result = rmalloc(sizeof(where_condition));
    result->type = CONDITION_AND;
    result->and.first = first;
    result->and.second = second;
    return result;
}

/// THROWS: [MALLOC_EXCEPTION]
where_condition *where_condition_or(where_condition *first, where_condition *second) {
    where_condition *result = rmalloc(sizeof(where_condition));
    result->type = CONDITION_OR;
    result->or.first = first;
    result->or.second = second;
    return result;
}

/// THROWS: [MALLOC_EXCEPTION]
where_condition *where_condition_not(where_condition *first) {
    where_condition *result = rmalloc(sizeof(where_condition));
    result->type = CONDITION_NOT;
    result->not.first = first;
    return result;
}

static void operand_free(operand op) {
    switch (op.type) {
        case OPERAND_VALUE_LITERAL:
            break;
        case OPERAND_VALUE_COLUMN:
            switch (op.column.type) {
                case COLUMN_DESC_NAME:
                    free(op.column.name.table_name);
                    free(op.column.name.column_name);
                    break;
                case COLUMN_DESC_INDEX:
                    break;
            }
    }
}

static void compare_condition_free(where_condition *condition) {
    assert(condition->type == CONDITION_COMPARE);
    operand_free(condition->compare.first);
    operand_free(condition->compare.second);
}

void where_condition_free(where_condition *condition) {
    switch (condition->type) {
        case CONDITION_AND:
            where_condition_free(condition->and.first);
            where_condition_free(condition->and.second);
            break;
        case CONDITION_OR:
            where_condition_free(condition->or.first);
            where_condition_free(condition->or.second);
            break;
        case CONDITION_NOT:
            where_condition_free(condition->not.first);
            break;
        case CONDITION_COMPARE:
            compare_condition_free(condition);
            break;
    }
    free(condition);
}

bool columns_equals(Column first, Column second) {
    assert(first.type == second.type);
    switch (first.type) {
        case COLUMN_TYPE_INT:
            return first.value.i32 == second.value.i32;
        case COLUMN_TYPE_FLOAT:
            return first.value.f32 == second.value.f32;
        case COLUMN_TYPE_STRING:
            return 0 == strcmp(first.value.str, second.value.str);
        case COLUMN_TYPE_BOOL:
            return first.value.b8 == second.value.b8;
    }
    assert(0);
}
