//
// Created by wieceslaw on 23.12.23.
//

#ifndef SINGLE_FILE_DATABASE_ROW_H
#define SINGLE_FILE_DATABASE_ROW_H

#include "Column.h"
#include "buffer/Buffer.h"

typedef struct Row {
    Column *columns;
    size_t size;
} Row;

Row RowCopy(Row row);

void RowFree(Row row);

size_t RowSize(Row row);

Buffer RowSerialize(Row row);

#endif //SINGLE_FILE_DATABASE_ROW_H
