//
// Created by wieceslaw on 28.12.23.
//

#include <stdbool.h>
#include <assert.h>
#include <string.h>
#include <malloc.h>
#include "Column.h"
#include "util_string.h"

Column ColumnCopy(Column column) {
    Column copy;
    if (column.type == COLUMN_TYPE_STRING) {
        copy.value.str = string_copy(column.value.str);
    } else {
        copy.value = column.value;
    }
    copy.type = column.type;
    return copy;
}

void ColumnFree(Column column) {
    switch (column.type) {
        case COLUMN_TYPE_INT32:
        case COLUMN_TYPE_FLOAT32:
        case COLUMN_TYPE_BOOL:
            break;
        case COLUMN_TYPE_STRING:
            free(column.value.str);
    }
}

bool ColumnEquals(Column first, Column second) {
    assert(first.type == second.type);
    switch (first.type) {
        case COLUMN_TYPE_INT32:
            return first.value.i32 == second.value.i32;
        case COLUMN_TYPE_FLOAT32:
            return first.value.f32 == second.value.f32;
        case COLUMN_TYPE_STRING:
            return 0 == strcmp(first.value.str, second.value.str);
        case COLUMN_TYPE_BOOL:
            return first.value.b8 == second.value.b8;
    }
    if (first.type == 0 && second.type == 0) {
        return false;
    }
    assert(0);
}

Column ColumnOfInt32(int32_t value) {
    return (Column) {
            .type = COLUMN_TYPE_INT32,
            .value = {.i32 = value}
    };
}

Column ColumnOfFloat32(float value) {
    return (Column) {
            .type = COLUMN_TYPE_FLOAT32,
            .value = {.f32 = value}
    };
}

Column ColumnOfString(char *value) {
    return (Column) {
            .type = COLUMN_TYPE_STRING,
            .value = {.str = value}
    };
}

Column ColumnOfBool(bool value) {
    return (Column) {
            .type = COLUMN_TYPE_BOOL,
            .value = {.b8 = value}
    };
}
