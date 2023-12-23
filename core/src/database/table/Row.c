//
// Created by wieceslaw on 23.12.23.
//

#include <stddef.h>
#include <assert.h>
#include <string.h>
#include <malloc.h>
#include "Row.h"
#include "util_string.h"

size_t RowSize(Row row) {
    assert(row.columns != NULL);
    size_t size = 0;
    for (uint32_t i = 0; i < row.size; i++) {
        Column column = row.columns[i];
        switch (column.type) {
            case COLUMN_TYPE_INT:
                size += sizeof(b32_t);
                break;
            case COLUMN_TYPE_FLOAT:
                size += sizeof(b32_t);
                break;
            case COLUMN_TYPE_STRING:
                size += strlen(column.value.str) + 1;
                break;
            case COLUMN_TYPE_BOOL:
                size += sizeof(b8_t);
                break;
        }
    }
    return size;
}

void RowFree(Row row) {
    if (row.columns == NULL) {
        return;
    }
    for (size_t i = 0; i < row.size; i++) {
        Column col = row.columns[i];
        switch (col.type) {
            case COLUMN_TYPE_INT:
            case COLUMN_TYPE_FLOAT:
            case COLUMN_TYPE_BOOL:
                break;
            case COLUMN_TYPE_STRING:
                free(col.value.str);
        }
    }
    free(row.columns);
    row.columns = NULL;
}

buffer_t RowSerialize(Row row) {
    assert(row.columns != NULL);
    buffer_t buffer = buffer_init(RowSize(row));
    for (uint32_t i = 0; i < row.size; i++) {
        Column col = row.columns[i];
        switch (col.type) {
            case COLUMN_TYPE_INT:
                buffer_write_b32(buffer, (b32_t) {.i32 = col.value.i32});
                break;
            case COLUMN_TYPE_FLOAT:
                buffer_write_b32(buffer, (b32_t) {.f32 = col.value.f32});
                break;
            case COLUMN_TYPE_STRING:
                buffer_write_string(buffer, col.value.str);
                break;
            case COLUMN_TYPE_BOOL:
                buffer_write_b8(buffer, (b8_t) {.ui8 = col.value.b8});
                break;
        }
    }
    return buffer;
}

Row RowCopy(Row row) {
    Row copy;
    copy.size = row.size;
    copy.columns = malloc(sizeof(Column) * copy.size);
    if (copy.columns == NULL) {
        assert(0);
    }
    for (size_t i = 0; i < row.size; i++) {
        copy.columns[i] = ColumnCopy(row.columns[i]);
    }
    return copy;
}

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

Column ColumnOfInt32(int32_t value) {
    return (Column) {
            .type = COLUMN_TYPE_INT,
            .value = {.i32 = value}
    };
}

Column ColumnOfFloat32(float value) {
    return (Column) {
            .type = COLUMN_TYPE_FLOAT,
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
