//
// Created by vyach on 17.09.2023.
//

#ifndef LLP_LAB1_TABLE_H
#define LLP_LAB1_TABLE_H

#include "util/defines.h"
#include "pool/pool.h"
#include "row.h"

typedef struct result_set_t result_set_t;

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

void scheme_free(table_scheme *scheme);

void table_free(table_t *table);

#endif //LLP_LAB1_TABLE_H
