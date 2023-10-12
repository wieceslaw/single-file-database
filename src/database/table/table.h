//
// Created by vyach on 17.09.2023.
//

#ifndef LLP_LAB1_TABLE_H
#define LLP_LAB1_TABLE_H

#include "util/defines.h"
#include "pool/pool.h"
#include "stddef.h"

typedef enum {
    COLUMN_TYPE_INT = 1,
    COLUMN_TYPE_FLOAT = 2,
    COLUMN_TYPE_STRING = 3,
    COLUMN_TYPE_BOOL = 4
} column_type;

typedef union {
    float val_float;
    int32_t val_int;
    uint8_t val_bool;
    char *val_string;
} column_value;

typedef struct row_value {
    column_value *values;
} *row_value;

typedef struct {
    column_type type;
    column_value value;
} column;

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

void row_value_free(const table_scheme *const scheme, row_value row);

buffer_t row_serialize(const table_scheme *scheme, row_value row);

row_value row_deserialize(const table_scheme *scheme, buffer_t buffer);

void table_scheme_free(table_scheme *scheme);

void table_free(table_t *table);

column_description table_column_of(char *table_name, char *column_name);

#endif //LLP_LAB1_TABLE_H
