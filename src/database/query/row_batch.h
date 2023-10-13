//
// Created by vyach on 12.10.2023.
//

#ifndef LLP_LAB1_ROW_BATCH_H
#define LLP_LAB1_ROW_BATCH_H

#include "util/vector/vector.h"
#include "database/table/table.h"

typedef struct row_builder {
    column_t *columns;
    size_t size;
    size_t capacity;
} row_builder_t;

row_builder_t row_builder_init(size_t capacity);

void row_builder_free(row_builder_t *row_builder);

void row_builder_add(row_builder_t *row_builder, column_t col);

row_t row_builder_to_row(row_builder_t *row_builder);

typedef struct batch_builder {
    row_builder_t *rows;
    size_t size;
    size_t capacity;
} batch_builder_t;

batch_builder_t batch_builder_init(size_t capacity);

void batch_builder_free(batch_builder_t *batch_builder);

void batch_builder_add(batch_builder_t *batch_builder, row_builder_t row);

#endif //LLP_LAB1_ROW_BATCH_H
