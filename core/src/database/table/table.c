//
// Created by vyach on 01.10.2023.
//

#include <assert.h>
#include <string.h>
#include <malloc.h>
#include "table.h"
#include "util_string.h"
#include "exceptions/exceptions.h"

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

// THROWS: [POOL_EXCEPTION]
void table_free(table_t *table_ptr) {
    assert(table_ptr != NULL);
    table_t table = *table_ptr;
    if (NULL == table) {
        return;
    }
    TRY({
                if (pool_free(table->data_pool) != 0) {
                    RAISE(POOL_EXCEPTION);
                }
        })
    FINALLY({
                    table_scheme_free(table->scheme);
            table->scheme = NULL;
            free(table);
            *table_ptr = NULL;
            })
}

Row row_deserialize(table_scheme *scheme, Buffer buffer) {
    assert(scheme != NULL && buffer != NULL);
    Column *columns = malloc(sizeof(Column) * scheme->size);
    if (columns == NULL) {
        return (Row) {0};
    }
    for (uint32_t i = 0; i < scheme->size; i++) {
        table_scheme_column scheme_col = scheme->columns[i];
        switch (scheme_col.type) {
            case COLUMN_TYPE_INT:
                columns[i].value.i32 = BufferReadB32(buffer).i32;
                break;
            case COLUMN_TYPE_FLOAT:
                columns[i].value.f32 = BufferReadB32(buffer).f32;
                break;
            case COLUMN_TYPE_STRING: {
                columns[i].value.str = BufferReadString(buffer);
                break;
            }
            case COLUMN_TYPE_BOOL:
                columns[i].value.b8 = BufferReadB8(buffer).ui8;
                break;
        }
        columns[i].type = scheme_col.type;
    }
    return (Row) {
            .size = scheme->size,
            .columns = columns
    };
}

static void read_columns(table_scheme *schema, Buffer buffer) {
    for (uint32_t i = 0; i < schema->size; i++) {
        ColumnType col_type = BufferReadB32(buffer).ui32;
        char *col_name = BufferReadString(buffer);
        schema->columns[i] = (table_scheme_column) {.type = col_type, .name = col_name};
    }
}

table_scheme *table_scheme_deserialize(Buffer buffer) {
    assert(buffer != NULL);
    // table[offset_t pool_offset, name*, table_scheme[size, values[type, name*]*]*]*
    BufferReset(buffer);
    table_scheme *scheme = malloc(sizeof(table_scheme));
    if (scheme == NULL) {
        return NULL;
    }
    scheme->pool_offset = BufferReadB64(buffer).ui64;
    scheme->name = BufferReadString(buffer); // TODO: may be null
    scheme->size = BufferReadB32(buffer).ui32;
    scheme->columns = malloc(sizeof(table_scheme_column) * scheme->size);
    if (scheme->columns == NULL) {
        free(scheme->name);
        free(scheme);
        return NULL;
    }
    read_columns(scheme, buffer); // TODO: may be null
    return scheme;
}

static size_t table_scheme_size(table_scheme *table_scheme) {
    assert(table_scheme != NULL);
    size_t size = sizeof(table_scheme->pool_offset) + (strlen(table_scheme->name) + 1) + sizeof(table_scheme->size);
    for (uint32_t i = 0; i < table_scheme->size; i++) {
        table_scheme_column col = table_scheme->columns[i];
        size += (strlen(col.name) + 1) + sizeof(col.type);
    }
    return size;
}

table_scheme *table_scheme_copy(table_scheme *src) {
    table_scheme *copy = malloc(sizeof(table_scheme));
    if (copy == NULL) {
        return NULL;
    }
    copy->name = string_copy(src->name);
    if (copy->name == NULL) {
        free(copy);
        return NULL;
    }
    copy->pool_offset = src->pool_offset;
    copy->size = src->size;
    copy->columns = malloc(sizeof(table_scheme_column) * copy->size);
    if (copy->columns == NULL) {
        free(copy->name);
        free(copy);
        return NULL;
    }
    for (size_t i = 0; i < copy->size; i++) {
        copy->columns[i].type = src->columns[i].type;
        copy->columns[i].name = string_copy(src->columns[i].name);
        if (copy->columns[i].name == NULL) {
            // TODO: free all columns before
            free(copy->name);
            free(copy);
            return NULL;
        }
    }
    return copy;
}

Buffer table_scheme_serialize(table_scheme *table_scheme) {
    assert(table_scheme != NULL);
    uint64_t size = table_scheme_size(table_scheme);
    Buffer buffer = BufferNew(size);
    BufferReset(buffer);
    if (NULL == buffer) {
        return NULL;
    }
    BufferWriteB64(buffer, (b64_t) {.ui64 = table_scheme->pool_offset});
    BufferWriteString(buffer, table_scheme->name);
    BufferWriteB32(buffer, (b32_t) {.ui32 = table_scheme->size});
    for (uint32_t i = 0; i < table_scheme->size; i++) {
        table_scheme_column col = table_scheme->columns[i];
        BufferWriteB32(buffer, (b32_t) {.i32 = col.type});
        BufferWriteString(buffer, col.name);
    }
    return buffer;
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
