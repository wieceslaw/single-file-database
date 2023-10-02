//
// Created by vyach on 17.09.2023.
//

#ifndef LLP_LAB1_TABLE_H
#define LLP_LAB1_TABLE_H

#include <stdint-gcc.h>
#include "pool/pool.h"

typedef enum column_type {
    TABLE_COLUMN_INT = 0,
    TABLE_COLUMN_FLOAT = 1,
    TABLE_COLUMN_STRING = 2,
    TABLE_COLUMN_BOOL = 3
} column_type_t;

typedef struct column {
    char *name;
    column_type_t type;
} column_t;

typedef struct table_schema {
    char *name;
    uint32_t size;
    column_t *columns;
    offset_t pool_offset;
} table_schema_t;

typedef struct table {
    table_schema_t *schema;
    pool_t *data_pool;
} table_t;


void free_schema(table_schema_t *schema);

void table_free(table_t *table);

#endif //LLP_LAB1_TABLE_H
