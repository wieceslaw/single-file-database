//
// Created by vyach on 17.09.2023.
//

#ifndef LLP_LAB1_TABLE_H
#define LLP_LAB1_TABLE_H

#include "util/defines.h"
#include "pool/pool.h"
#include "stddef.h"

typedef enum column_type {
    COLUMN_TYPE_INT = 1,
    COLUMN_TYPE_FLOAT = 2,
    COLUMN_TYPE_STRING = 3,
    COLUMN_TYPE_BOOL = 4
} column_type;

typedef union column_value {
    float val_float;
    int32_t val_int;
    uint8_t val_bool;
    char *val_string;
} column_value;

typedef struct column {
    column_type type;
    column_value value;
} column_t;

typedef struct row {
    column_t *columns;
    size_t size;
} row_t;

typedef enum column_description_type {
    COLUMN_DESC_NAME = 0,
    COLUMN_DESC_INDEX = 1,
} column_description_type;

typedef struct {
    column_description_type type;
    union {
        struct {
            char *table_name;
            char *column_name;
        } name;
        struct {
            size_t table_idx;
            size_t column_idx;
        } index;
    };
} column_description;

typedef struct table_scheme_column {
    char *name;
    column_type type;
} table_scheme_column;

typedef struct table_scheme {
    char *name;
    uint32_t size;
    table_scheme_column *columns;
    offset_t pool_offset;
} table_scheme;

typedef struct table {
    table_scheme *scheme;
    pool_t *data_pool;
} *table_t;

void row_free(row_t row);

column_t column_copy(column_t column);

row_t row_copy(row_t row);

buffer_t row_serialize(row_t row);

row_t row_deserialize(const table_scheme *scheme, buffer_t buffer);

void table_scheme_free(table_scheme *scheme);

void table_free(table_t *table);

column_description table_column_of(char *table_name, char *column_name);

column_t column_int(int32_t value);

column_t column_float(float value);

column_t column_string(char *value);

column_t column_bool(bool value);

#endif //LLP_LAB1_TABLE_H
