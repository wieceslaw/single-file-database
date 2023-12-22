//
// Created by wieceslaw on 23.12.23.
//

#ifndef SINGLE_FILE_DATABASE_ROWBUILDER_H
#define SINGLE_FILE_DATABASE_ROWBUILDER_H

#include <stddef.h>
#include "database/table/table.h"

typedef struct RowBuilder {
    column_t *columns;
    size_t size;
    size_t capacity;
} RowBuilder;

RowBuilder RowBuilderNew(size_t capacity);

void RowBuilderFree(RowBuilder *builder);

void RowBuilderAdd(RowBuilder *builder, column_t col);

row_t RowBuilderToRow(RowBuilder *builder);

#endif //SINGLE_FILE_DATABASE_ROWBUILDER_H
