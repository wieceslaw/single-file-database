//
// Created by vyach on 13.10.2023.
//

#include <assert.h>
#include "RowBatch.h"
#include "exceptions/exceptions.h"

RowBatch RowBatchNew(size_t capacity) {
    return (RowBatch) {
        .capacity = capacity,
        .size = 0,
        .rows = rmalloc(sizeof(Row) * capacity)
    };
}

void RowBatchFree(RowBatch *batch) {
    assert(batch != NULL);
    if (NULL == batch->rows) {
        return;
    }
    for (size_t i = 0; i < batch->size; i++) {
        RowFree(batch->rows[i]);
    }
    free(batch->rows);
    batch->rows = NULL;
}

void RowBatchAddRow(RowBatch *batch, Row row) {
    assert(batch != NULL);
    if (batch->size == batch->capacity) {
        return;
    }
    batch->rows[batch->size] = row;
    batch->size++;
}
