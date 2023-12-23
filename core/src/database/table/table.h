//
// Created by vyach on 17.09.2023.
//

#ifndef LLP_LAB1_TABLE_H
#define LLP_LAB1_TABLE_H

#include "stddef.h"
#include "defines.h"
#include "pool/pool.h"
#include "Row.h"

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
    ColumnType type;
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

void table_scheme_free(table_scheme *scheme);

table_scheme* table_scheme_copy(table_scheme* table_scheme);

table_scheme *table_scheme_deserialize(Buffer buffer);

Buffer table_scheme_serialize(table_scheme *table_scheme);

void table_free(table_t *table);

column_description table_column_of(char *table_name, char *column_name);

Row row_deserialize(table_scheme *scheme, Buffer buffer);

#endif //LLP_LAB1_TABLE_H
