//
// Created by vyach on 17.09.2023.
//

#ifndef LLP_LAB1_TABLE_H
#define LLP_LAB1_TABLE_H

#include <stdint-gcc.h>
#include "pool/pool.h"

typedef enum {
    TABLE_OP_OK = 0,
    TABLE_OP_ERR = 1,
} table_result_type;

typedef enum {
    TYPE_INT = 0,
    TYPE_FLOAT = 1,
    TYPE_STRING = 2,
    TYPE_BOOL = 3
} column_type;

typedef struct result_set_t result_set_t;

typedef struct {
    table_result_type type;
    result_set_t *result;
} table_result_t;

typedef struct {
    char *name;
    column_type type;
} scheme_column_t;

typedef struct {
    char *name;
    uint32_t size;
    scheme_column_t *columns;
    offset_t pool_offset;
} scheme_t;

typedef struct {
    scheme_t *scheme;
    pool_t *data_pool;
} table_t;

void scheme_free(scheme_t *scheme);

table_result_type table_free(table_t *table);

#endif //LLP_LAB1_TABLE_H
