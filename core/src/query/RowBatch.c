//
// Created by vyach on 13.10.2023.
//

#include <assert.h>
#include <malloc.h>
#include "RowBatch.h"

RowBatch RowBatchNew(size_t capacity) {
    return (RowBatch) {
        .capacity = capacity,
        .size = 0,
        .rows = malloc(sizeof(Row) * capacity)
    };
}

void RowBatchFree(RowBatch *batch) {
    assert(batch != NULL);
    if (batch->rows == NULL) {
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
