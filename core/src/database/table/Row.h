//
// Created by wieceslaw on 23.12.23.
//

#ifndef SINGLE_FILE_DATABASE_ROW_H
#define SINGLE_FILE_DATABASE_ROW_H

#include "buffer/Buffer.h"

typedef enum ColumnType {
    COLUMN_TYPE_INT = 0,
    COLUMN_TYPE_FLOAT = 1,
    COLUMN_TYPE_STRING = 2,
    COLUMN_TYPE_BOOL = 3
} ColumnType;

typedef union ColumnValue {
    float f32;
    int32_t i32;
    uint8_t b8;
    char *str;
} ColumnValue;

typedef struct column {
    ColumnType type;
    ColumnValue value;
} Column;

typedef struct row {
    Column *columns;
    size_t size;
} Row;

Column ColumnOfInt32(int32_t value);

Column ColumnOfFloat32(float value);

Column ColumnOfString(char *value);

Column ColumnOfBool(bool value);

Column ColumnCopy(Column column);

void RowFree(Row row);

Row RowCopy(Row row);

size_t RowSize(Row row);

Buffer RowSerialize(Row row);

#endif //SINGLE_FILE_DATABASE_ROW_H
