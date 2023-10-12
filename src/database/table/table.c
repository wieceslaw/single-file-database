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

static uint64_t row_size(const table_scheme *const scheme, row_value row) {
    assert(scheme != NULL && row != NULL);
    uint64_t size = sizeof(scheme->size);
    for (uint32_t i = 0; i < scheme->size; i++) {
        table_scheme_column scheme_col = scheme->columns[i];
        switch (scheme_col.type) {
            case COLUMN_TYPE_INT:
                size += sizeof(b32_t);
                break;
            case COLUMN_TYPE_FLOAT:
                size += sizeof(b32_t);
                break;
            case COLUMN_TYPE_STRING:
                size += strlen(row->values[i].val_string);
                break;
            case COLUMN_TYPE_BOOL:
                size += sizeof(b8_t);
                break;
        }
    }
    return size;
}

void row_value_free(const table_scheme *const scheme, row_value row) {
    for (uint32_t i = 0; i < scheme->size; i++) {
        table_scheme_column scheme_col = scheme->columns[i];
        column_value col = row->values[i];
        switch (scheme_col.type) {
            case COLUMN_TYPE_INT:
            case COLUMN_TYPE_FLOAT:
            case COLUMN_TYPE_BOOL:
                break;
            case COLUMN_TYPE_STRING:
                free(col.val_string);
        }
    }
    free(row->values);
    free(row);
}

// THROWS: [MALLOC_EXCEPTION]
buffer_t row_serialize(const table_scheme *const scheme, row_value row) {
    assert(scheme != NULL && row != NULL);
    buffer_t buffer = buffer_init(row_size(scheme, row));
    for (uint32_t i = 0; i < scheme->size; i++) {
        table_scheme_column scheme_col = scheme->columns[i];
        column_value col = row->values[i];
        switch (scheme_col.type) {
            case COLUMN_TYPE_INT:
                buffer_write_b32(buffer, (b32_t) {.i32 = col.val_int});
                break;
            case COLUMN_TYPE_FLOAT:
                buffer_write_b32(buffer, (b32_t) {.f32 = col.val_float});
                break;
            case COLUMN_TYPE_STRING:
                buffer_write_string(buffer, col.val_string);
                break;
            case COLUMN_TYPE_BOOL:
                buffer_write_b8(buffer, (b8_t) {.ui8 = col.val_bool});
                break;
        }
    }
    return buffer;
}

// THROWS: [MALLOC_EXCEPTION]
row_value row_deserialize(const table_scheme *const scheme, buffer_t buffer) {
    assert(scheme != NULL && buffer != NULL);
    row_value row = rmalloc(sizeof(struct row_value));
    row->values = rmalloc(sizeof(column_value) * scheme->size);
    for (uint32_t i = 0; i < scheme->size; i++) {
        table_scheme_column scheme_col = scheme->columns[i];
        switch (scheme_col.type) {
            case COLUMN_TYPE_INT:
                row->values[i].val_int = buffer_read_b32(buffer).i32;
                break;
            case COLUMN_TYPE_FLOAT:
                row->values[i].val_float = buffer_read_b32(buffer).f32;
                break;
            case COLUMN_TYPE_STRING: {
                row->values[i].val_string = buffer_read_string(buffer);
                break;
            }
            case COLUMN_TYPE_BOOL:
                row->values[i].val_bool = buffer_read_b8(buffer).ui8;
                break;
        }
    }
    return row;
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
