//
// Created by vyach on 03.10.2023.
//

#ifndef LLP_LAB1_ROW_H
#define LLP_LAB1_ROW_H

#include "util/defines.h"
#include "buffer/buffer.h"
#include "table.h"

typedef enum {
    TYPE_INT = 0,
    TYPE_FLOAT = 1,
    TYPE_STRING = 2,
    TYPE_BOOL = 3
} column_type;

typedef union {
    float val_float;
    int32_t val_int;
    uint8_t val_bool;
    char *val_string;
} column_value;

typedef struct {
    column_type type;
    column_value value;
} column;

typedef struct row {
    column_value *values;
} *row_value;

buffer_t row_serialize(const table_scheme *scheme, row_value row);

row_value row_deserialize(const table_scheme *scheme, buffer_t buffer);

bool columns_equal(column col1, column col2);

#endif //LLP_LAB1_ROW_H
