//
// Created by vyach on 13.10.2023.
//

#include <assert.h>
#include "row_batch.h"
#include "exceptions/exceptions.h"
#include "util_string.h"

row_builder_t row_builder_init(size_t capacity) {
    return (row_builder_t) {
        .size = 0,
        .capacity = capacity,
        .columns = rmalloc(sizeof(column_t) * capacity)
    };
}

void row_builder_free(row_builder_t *row_builder) {
    assert(row_builder != NULL);
    if (NULL == row_builder->columns) {
        return;
    }
    for (size_t i = 0; i < row_builder->size; i++) {
        column_t col = row_builder->columns[i];
        if (col.type == COLUMN_TYPE_STRING) {
            free(col.value.val_string);
        }
    }
    free(row_builder->columns);
    row_builder->columns = NULL;
}

void row_builder_add(row_builder_t *row_builder, column_t col) {
    assert(row_builder != NULL);
    if (row_builder->size == row_builder->capacity) {
        return;
    }
    column_t copy;
    copy.type = col.type;
    if (col.type == COLUMN_TYPE_STRING) {
        copy.value.val_string = string_copy(col.value.val_string);
    } else {
        copy.value = col.value;
    }
    row_builder->columns[row_builder->size] = copy;
    row_builder->size++;
}

batch_builder_t batch_builder_init(size_t capacity) {
    return (batch_builder_t) {
        .capacity = capacity,
        .size = 0,
        .rows = rmalloc(sizeof(row_builder_t) * capacity)
    };
}

void batch_builder_free(batch_builder_t *batch_builder) {
    assert(batch_builder != NULL);
    if (NULL == batch_builder->rows) {
        return;
    }
    for (size_t i = 0; i < batch_builder->size; i++) {
        row_builder_free(&batch_builder->rows[i]);
    }
    free(batch_builder->rows);
    batch_builder->rows = NULL;
}

void batch_builder_add(batch_builder_t *batch_builder, row_builder_t row) {
    assert(batch_builder != NULL);
    if (batch_builder->size == batch_builder->capacity) {
        return;
    }
    batch_builder->rows[batch_builder->size] = row;
    batch_builder->size++;
}

row_t row_builder_as_row(row_builder_t *row_builder) {
    return (row_t) {
        .size = row_builder->size,
        .columns = row_builder->columns
    };
}
