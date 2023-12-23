//
// Created by wieceslaw on 23.12.23.
//

#ifndef SINGLE_FILE_DATABASE_ROWBUILDER_H
#define SINGLE_FILE_DATABASE_ROWBUILDER_H

#include <stddef.h>
#include "database/table/table.h"

typedef struct RowBuilder {
    Column *columns;
    size_t size;
    size_t capacity;
} RowBuilder;

RowBuilder RowBuilderNew(size_t capacity);

void RowBuilderFree(RowBuilder *builder);

void RowBuilderAdd(RowBuilder *builder, Column col);

Row RowBuilderToRow(RowBuilder *builder);

#endif //SINGLE_FILE_DATABASE_ROWBUILDER_H
