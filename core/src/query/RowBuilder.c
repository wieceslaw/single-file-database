//
// Created by wieceslaw on 23.12.23.
//

#include <assert.h>
#include <malloc.h>
#include "RowBuilder.h"
#include "util_string.h"

RowBuilder RowBuilderNew(size_t capacity) {
    column_t *columns = malloc(sizeof(column_t) * capacity);
    if (columns == NULL) {
        return (RowBuilder) {0};
    }
    return (RowBuilder) {
            .size = 0,
            .capacity = capacity,
            .columns = columns
    };
}

void RowBuilderFree(RowBuilder *builder) {
    assert(builder != NULL);
    if (NULL == builder->columns) {
        return;
    }
    for (size_t i = 0; i < builder->size; i++) {
        column_t col = builder->columns[i];
        if (col.type == COLUMN_TYPE_STRING) {
            free(col.value.val_string);
        }
    }
    free(builder->columns);
    builder->columns = NULL;
}

void RowBuilderAdd(RowBuilder *builder, column_t col) {
    assert(builder != NULL);
    if (builder->size == builder->capacity) {
        return;
    }
    column_t copy;
    copy.type = col.type;
    if (col.type == COLUMN_TYPE_STRING) {
        copy.value.val_string = string_copy(col.value.val_string);
    } else {
        copy.value = col.value;
    }
    builder->columns[builder->size] = copy;
    builder->size++;
}

row_t RowBuilderToRow(RowBuilder *builder) {
    debug("Fix (allocate new row)");
    assert(0);
    return (row_t) {
            .size = builder->size,
            .columns = builder->columns
    };
}
