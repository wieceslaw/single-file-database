//
// Created by wieceslaw on 23.12.23.
//

#include <assert.h>
#include <malloc.h>
#include "RowBuilder.h"
#include "util_string.h"

RowBuilder RowBuilderNew(size_t capacity) {
    Column *columns = malloc(sizeof(Column) * capacity);
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
        Column col = builder->columns[i];
        if (col.type == COLUMN_TYPE_STRING) {
            free(col.value.str);
        }
    }
    free(builder->columns);
    builder->columns = NULL;
}

void RowBuilderAdd(RowBuilder *builder, Column col) {
    assert(builder != NULL);
    if (builder->size == builder->capacity) {
        return;
    }
    Column copy;
    copy.type = col.type;
    if (col.type == COLUMN_TYPE_STRING) {
        copy.value.str = string_copy(col.value.str);
    } else {
        copy.value = col.value;
    }
    builder->columns[builder->size] = copy;
    builder->size++;
}

Row RowBuilderToRow(RowBuilder *builder) {
    debug("Fix (allocate new row)");
    assert(0);
    return (Row) {
            .size = builder->size,
            .columns = builder->columns
    };
}
