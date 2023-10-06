//
// Created by vyach on 03.10.2023.
//

#ifndef LLP_LAB1_ROW_H
#define LLP_LAB1_ROW_H

#include "util/defines.h"
#include "buffer/buffer.h"
#include "table.h"

typedef union {
    float val_float;
    int32_t val_int;
    uint8_t val_bool;
    char *val_string;
} column;

typedef struct row {
    column *columns;
    uint32_t size;
} *row_t;

buffer_t row_serialize(const table_scheme *scheme, row_t row);

row_t row_deserialize(const table_scheme *scheme, buffer_t buffer);

#endif //LLP_LAB1_ROW_H
