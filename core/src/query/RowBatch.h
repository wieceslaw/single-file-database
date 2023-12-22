//
// Created by vyach on 12.10.2023.
//

#ifndef LLP_LAB1_ROW_BATCH_H
#define LLP_LAB1_ROW_BATCH_H

#include "vector/vector.h"
#include "database/table/table.h"

typedef struct RowBatch {
    row_t *rows;
    size_t size;
    size_t capacity;
} RowBatch;

RowBatch RowBatchNew(size_t capacity);

void RowBatchFree(RowBatch *batch);

void RowBatchAddRow(RowBatch *batch, row_t row);

#endif //LLP_LAB1_ROW_BATCH_H
