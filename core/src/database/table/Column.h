//
// Created by wieceslaw on 28.12.23.
//

#ifndef SINGLE_FILE_DATABASE_COLUMN_H
#define SINGLE_FILE_DATABASE_COLUMN_H

#include <stdint-gcc.h>
#include <stdbool.h>

typedef enum ColumnType {
    COLUMN_TYPE_INT32 = 1,
    COLUMN_TYPE_FLOAT32 = 2,
    COLUMN_TYPE_STRING = 3,
    COLUMN_TYPE_BOOL = 4
} ColumnType;

typedef union ColumnValue {
    float f32;
    int32_t i32;
    uint8_t b8;
    char *str;
} ColumnValue;

typedef struct Column {
    ColumnType type;
    ColumnValue value;
} Column;

Column ColumnCopy(Column column);

void ColumnFree(Column column);

bool ColumnEquals(Column first, Column second);

Column ColumnOfInt32(int32_t value);

Column ColumnOfFloat32(float value);

Column ColumnOfString(char *value);

Column ColumnOfBool(bool value);

#endif //SINGLE_FILE_DATABASE_COLUMN_H
