//
// Created by vyach on 01.10.2023.
//

#include <assert.h>
#include <string.h>
#include "table.h"
#include "util/exceptions/exceptions.h"
#include "util/string.h"

static void free_column(table_scheme_column *column) {
    assert(column != NULL);
    free(column->name);
    column->name = NULL;
}

void table_scheme_free(table_scheme *scheme) {
    if (NULL == scheme) {
        return;
    }
    for (uint32_t i = 0; i < scheme->size; i++) {
        free_column(&scheme->columns[i]);
    }
    free(scheme->columns);
    scheme->columns = NULL;
    free(scheme->name);
    scheme->name = NULL;
    free(scheme);
}

/// THROWS: [POOL_EXCEPTION]
void table_free(table_t *table_ptr) {
    assert(table_ptr != NULL);
    table_t table = *table_ptr;
    if (NULL == table) {
        return;
    }
    TRY ({
        pool_free(table->data_pool);
    }) FINALLY({
        table_scheme_free(table->scheme);
        table->scheme = NULL;
        free(table);
        *table_ptr = NULL;
    })
}

static uint64_t row_size(row_t row) {
    assert(row.columns != NULL);
    size_t size = 0;
    for (uint32_t i = 0; i < row.size; i++) {
        column_t column = row.columns[i];
        switch (column.type) {
            case COLUMN_TYPE_INT:
                size += sizeof(b32_t);
                break;
            case COLUMN_TYPE_FLOAT:
                size += sizeof(b32_t);
                break;
            case COLUMN_TYPE_STRING:
                size += strlen(column.value.val_string);
                break;
            case COLUMN_TYPE_BOOL:
                size += sizeof(b8_t);
                break;
        }
    }
    return size;
}

void row_free(row_t row) {
    if (NULL == row.columns) {
        return;
    }
    for (uint32_t i = 0; i < row.size; i++) {
        column_t col = row.columns[i];
        switch (col.type) {
            case COLUMN_TYPE_INT:
            case COLUMN_TYPE_FLOAT:
            case COLUMN_TYPE_BOOL:
                break;
            case COLUMN_TYPE_STRING:
                free(col.value.val_string);
        }
    }
    free(row.columns);
    row.columns = NULL;
}

// THROWS: [MALLOC_EXCEPTION]
buffer_t row_serialize(row_t row) {
    assert(row.columns != NULL);
    buffer_t buffer = buffer_init(row_size(row));
    for (uint32_t i = 0; i < row.size; i++) {
        column_t col = row.columns[i];
        switch (col.type) {
            case COLUMN_TYPE_INT:
                buffer_write_b32(buffer, (b32_t) {.i32 = col.value.val_int});
                break;
            case COLUMN_TYPE_FLOAT:
                buffer_write_b32(buffer, (b32_t) {.f32 = col.value.val_float});
                break;
            case COLUMN_TYPE_STRING:
                buffer_write_string(buffer, col.value.val_string);
                break;
            case COLUMN_TYPE_BOOL:
                buffer_write_b8(buffer, (b8_t) {.ui8 = col.value.val_bool});
                break;
        }
    }
    return buffer;
}

// THROWS: [MALLOC_EXCEPTION]
row_t row_deserialize(const table_scheme *scheme, buffer_t buffer) {
    assert(scheme != NULL && buffer != NULL);
    column_t *columns = rmalloc(sizeof(column_t) * scheme->size);
    for (uint32_t i = 0; i < scheme->size; i++) {
        table_scheme_column scheme_col = scheme->columns[i];
        switch (scheme_col.type) {
            case COLUMN_TYPE_INT:
                columns[i].value.val_int = buffer_read_b32(buffer).i32;
                break;
            case COLUMN_TYPE_FLOAT:
                columns[i].value.val_float = buffer_read_b32(buffer).f32;
                break;
            case COLUMN_TYPE_STRING: {
                columns[i].value.val_string = buffer_read_string(buffer);
                break;
            }
            case COLUMN_TYPE_BOOL:
                columns[i].value.val_bool = buffer_read_b8(buffer).ui8;
                break;
        }
        columns[i].type = scheme_col.type;
    }
    return (row_t) {
        .size = scheme->size,
        .columns = columns
    };
}

column_description table_column_of(char *table_name, char *column_name) {
    return (column_description) {
        .type = COLUMN_DESC_NAME,
        .name = {
            .table_name = string_copy(table_name),
            .column_name = string_copy(column_name)
        }
    };
}

column_t column_int(int32_t value) {
    return (column_t) {
        .type = COLUMN_TYPE_INT,
        .value = {.val_int = value}
    };
}

column_t column_float(float value) {
    return (column_t) {
            .type = COLUMN_TYPE_FLOAT,
            .value = {.val_float = value}
    };
}

column_t column_string(char *value) {
    return (column_t) {
            .type = COLUMN_TYPE_STRING,
            .value = {.val_string = value}
    };
}

column_t column_bool(bool value) {
    return (column_t) {
            .type = COLUMN_TYPE_BOOL,
            .value = {.val_bool = value}
    };
}
