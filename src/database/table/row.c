//
// Created by vyach on 03.10.2023.
//

#include <assert.h>
#include <string.h>
#include "row.h"
#include "util/exceptions/exceptions.h"

static uint64_t row_size(const table_scheme *const scheme, row_value row) {
    assert(scheme != NULL && row != NULL);
    uint64_t size = sizeof(row->size);
    for (uint32_t i = 0; i < scheme->size; i++) {
        table_scheme_column scheme_col = scheme->columns[i];
        switch (scheme_col.type) {
            case TYPE_INT:
                size += sizeof(b32_t);
                break;
            case TYPE_FLOAT:
                size += sizeof(b32_t);
                break;
            case TYPE_STRING:
                size += strlen(row->values[i].val_string);
                break;
            case TYPE_BOOL:
                size += sizeof(b8_t);
                break;
        }
    }
    return size;
}

// THROWS: [MALLOC_EXCEPTION]
buffer_t row_serialize(const table_scheme *const scheme, row_value row) {
    assert(scheme != NULL && row != NULL && row->size == scheme->size);
    buffer_t buffer = buffer_init(row_size(scheme, row));
    for (uint32_t i = 0; i < scheme->size; i++) {
        table_scheme_column scheme_col = scheme->columns[i];
        column_value col = row->values[i];
        switch (scheme_col.type) {
            case TYPE_INT:
                buffer_write_b32(buffer, (b32_t) {.i32 = col.val_int});
                break;
            case TYPE_FLOAT:
                buffer_write_b32(buffer, (b32_t) {.f32 = col.val_float});
                break;
            case TYPE_STRING:
                buffer_write_string(buffer, col.val_string);
                break;
            case TYPE_BOOL:
                buffer_write_b8(buffer, (b8_t) {.ui8 = col.val_bool});
                break;
        }
    }
    return buffer;
}

// THROWS: [MALLOC_EXCEPTION]
row_value row_deserialize(const table_scheme *const scheme, buffer_t buffer) {
    assert(scheme != NULL && buffer != NULL);
    row_value row = rmalloc(sizeof(column_value) * scheme->size);
    for (uint32_t i = 0; i < scheme->size; i++) {
        table_scheme_column scheme_col = scheme->columns[i];
        column_value *col = &(row->values[i]);
        switch (scheme_col.type) {
            case TYPE_INT:
                col->val_int = buffer_read_b32(buffer).i32;
                break;
            case TYPE_FLOAT:
                col->val_float = buffer_read_b32(buffer).f32;
                break;
            case TYPE_STRING:
                col->val_string = buffer_read_string(buffer);
                break;
            case TYPE_BOOL:
                col->val_bool = buffer_read_b8(buffer).ui8;
                break;
        }
    }
    return row;
}

bool columns_equal(column col1, column col2) {
    assert(col1.type == col2.type);
    switch (col1.type) {
        case TYPE_INT:
            return col1.value.val_int == col2.value.val_int;
        case TYPE_FLOAT:
            return col1.value.val_float == col2.value.val_float;
        case TYPE_STRING:
            return 0 == strcmp(col1.value.val_string, col2.value.val_string);
        case TYPE_BOOL:
            return col1.value.val_bool == col2.value.val_bool;
    }
}
