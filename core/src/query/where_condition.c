//
// Created by vyach on 06.10.2023.
//

#include <stdbool.h>
#include <assert.h>
#include <string.h>
#include <malloc.h>
#include "util_string.h"
#include "where_condition.h"

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
            .literal = {.type = COLUMN_TYPE_FLOAT32, .value = {.f32 = value}}
    };
}

operand operand_literal_int(int32_t value) {
    return (operand) {
            .type = OPERAND_VALUE_LITERAL,
            .literal = {.type = COLUMN_TYPE_INT32, .value = {.i32 = value}}
    };
}

operand operand_literal_bool(bool value) {
    return (operand) {
            .type = OPERAND_VALUE_LITERAL,
            .literal = {.type = COLUMN_TYPE_BOOL, .value = {.b8 = value}}
    };
}

operand operand_literal_string(char *value) {
    return (operand) {
            .type = OPERAND_VALUE_LITERAL,
            .literal = {.type = COLUMN_TYPE_STRING, .value = {.str = string_copy(value)}}
    };
}

where_condition *where_condition_compare(comparing_type type, operand op1, operand op2) {
    where_condition *result = malloc(sizeof(where_condition));
    if (result == NULL) {
        return NULL;
    }
    result->type = CONDITION_COMPARE;
    result->compare.type = type;
    result->compare.first = op1;
    result->compare.second = op2;
    return result;
}

where_condition *where_condition_and(where_condition *first, where_condition *second) {
    assert(first != NULL && second != NULL);
    where_condition *result = malloc(sizeof(where_condition));
    if (result == NULL) {
        return NULL;
    }
    result->type = CONDITION_AND;
    result->and.first = first;
    result->and.second = second;
    return result;
}

where_condition *where_condition_or(where_condition *first, where_condition *second) {
    assert(first != NULL && second != NULL);
    where_condition *result = malloc(sizeof(where_condition));
    if (result == NULL) {
        return NULL;
    }
    result->type = CONDITION_OR;
    result->or.first = first;
    result->or.second = second;
    return result;
}

where_condition *where_condition_not(where_condition *first) {
    assert(first != NULL);
    where_condition *result = malloc(sizeof(where_condition));
    if (result == NULL) {
        return NULL;
    }
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
    if (condition == NULL) {
        return;
    }
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
