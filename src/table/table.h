//
// Created by vyach on 17.09.2023.
//

#ifndef LLP_LAB1_TABLE_H
#define LLP_LAB1_TABLE_H

#include <stdint-gcc.h>
#include "pool/pool.h"

typedef enum column_type {
    COL_INT = 0,
    COL_FLOAT = 1,
    COL_STRING = 2,
    COL_BOOL = 3
} column_type_t;

typedef struct column {
    char* name;
    column_type_t type;
} column_t;

typedef struct schema {
    uint32_t size;
    column_t* columns;
} schema_t;

typedef struct table {
    char* name;
    schema_t *schema;
} table_t;

table_t *table_deserialize(buffer_t *buffer);

buffer_t *table_serialize(table_t *table);

void table_free(table_t *table);

#endif //LLP_LAB1_TABLE_H
