//
// Created by vyach on 02.10.2023.
//

#include <stddef.h>
#include <malloc.h>
#include <assert.h>
#include <string.h>
#include "allocator/allocator.h"
#include "pool/pool.h"
#include "table/table.h"
#include "database.h"
#include "database/table/row.h"
#include "util/exceptions/exceptions.h"

struct database {
    allocator_t *allocator;
    pool_t *table_pool;
};

static void read_columns(table_scheme *schema, buffer_t buffer) {
    for (uint32_t i = 0; i < schema->size; i++) {
        column_type col_type = buffer_read_b32(buffer).ui32;
        char *col_name = buffer_read_string(buffer);
        schema->columns[i] = (table_scheme_column) {.type = col_type, .name = col_name};
    }
}

/// THROWS: [MALLOC_EXCEPTION]
static table_scheme *table_schema_deserialize(buffer_t buffer) {
    assert(buffer != NULL);
    // table[offset_t pool_offset, name*, table_scheme[size, values[type, name*]*]*]*
    table_scheme *schema;
    TRY({
        buffer_reset(buffer);
        schema = rmalloc(sizeof(table_scheme));
        schema->pool_offset = buffer_read_b64(buffer).ui64;
        schema->name = buffer_read_string(buffer);
        schema->size = buffer_read_b32(buffer).ui32;
        schema->columns = rmalloc(sizeof(table_scheme_column) * schema->size);
        read_columns(schema, buffer);
    }) CATCH(exception >= EXCEPTION, {
        free(schema);
        free(schema->columns);
        RAISE(exception);
    }) FINALLY()
    return schema;
}

static uint64_t table_schema_length(const table_scheme *const table_schema) {
    assert(table_schema != NULL);
    uint64_t size = sizeof(table_schema->pool_offset) + (strlen(table_schema->name) + 1) + sizeof(table_schema->size);
    for (uint32_t i = 0; i < table_schema->size; i++) {
        table_scheme_column col = table_schema->columns[i];
        size += (strlen(col.name) + 1) + sizeof(col.type);
    }
    return size;
}

/// THROWS: [MALLOC_EXCEPTION]
static buffer_t table_serialize(const table_scheme *const table_schema) {
    assert(table_schema != NULL);
    uint64_t size = table_schema_length(table_schema);
    buffer_t buffer = buffer_init(size);
    buffer_reset(buffer);
    if (NULL == buffer) {
        return NULL;
    }
    buffer_write_b64(buffer, (b64_t) {.ui64 = table_schema->pool_offset});
    buffer_write_string(buffer, table_schema->name);
    buffer_write_b32(buffer, (b32_t) {.ui32 = table_schema->size});
    for (uint32_t i = 0; i < table_schema->size; i++) {
        table_scheme_column col = table_schema->columns[i];
        buffer_write_b32(buffer, (b32_t) {.i32 = col.type});
        buffer_write_string(buffer, col.name);
    }
    return buffer;
}

// THROWS: [MALLOC_EXCEPTION]
database_t database_init(file_settings *settings) {
    database_t database = rmalloc(sizeof(struct database));
    allocator_t *allocator;
    if (allocator_init(settings, &allocator) != FILE_ST_OK) {
        free(allocator);
        free(database);
        return NULL;
    }
    if (settings->open_type == FILE_OPEN_CLEAR || settings->open_type == FILE_OPEN_CREATE) {
        offset_t offset = pool_create(allocator);
        if (0 == offset) {
            allocator_free(allocator);
            free(database);
            return NULL;
        }
        allocator_set_entrypoint(allocator, offset);
    }
    pool_t *table_pool = pool_init(allocator, allocator_get_entrypoint(allocator));
    if (NULL == table_pool) {
        allocator_free(allocator);
        free(database);
        return NULL;
    }
    database->table_pool = table_pool;
    database->allocator = allocator;
    return database;
}

void database_free(database_t database) {
    TRY({
        pool_free(database->table_pool);
        allocator_free(database->allocator);
        free(database);
    }) CATCH(exception >= EXCEPTION, {
        allocator_free(database->allocator);
        free(database);
        RAISE(exception);
    }) FINALLY()
}

void database_create_table(database_t database, table_scheme *table_schema) {
    assert(database != NULL && table_schema != NULL);
    pool_it it = pool_iterator(database->table_pool);
    while (!pool_iterator_is_empty(it)) {
        buffer_t buffer = pool_iterator_get(it);
        assert(NULL == buffer);
        table_scheme *schema = table_schema_deserialize(buffer);
        if (0 == strcmp(schema->name, table_schema->name)) {
            pool_iterator_free(&it);
            RAISE(TABLE_ALREADY_EXISTS);
        }
        scheme_free(schema);
        buffer_free(&buffer);
        TRY({
            pool_iterator_next(it);
        }) CATCH(exception >= EXCEPTION, {
            pool_iterator_free(&it);
            RAISE(exception);
        }) FINALLY()
    }
    pool_iterator_free(&it);
    it = NULL;
    buffer_t serialized;
    TRY({
        offset_t offset = pool_create(database->allocator);
        table_schema->pool_offset = offset;
        serialized = table_serialize(table_schema);
        pool_append(database->table_pool, serialized);
        pool_flush(database->table_pool);
    }) CATCH(exception >= EXCEPTION, {
        buffer_free(&serialized);
    }) FINALLY()
}

void database_delete_table(database_t database, const char *const name) {
    pool_it it = NULL;
    buffer_t buffer = NULL;
    table_scheme *schema = NULL;
    TRY({
        it = pool_iterator(database->table_pool);
        while (!pool_iterator_is_empty(it)) {
            buffer = pool_iterator_get(it);
            assert(NULL != buffer);
            schema = table_schema_deserialize(buffer);
            if (0 == strcmp(schema->name, name)) {
                pool_iterator_delete(it);
                break;
            }
            scheme_free(schema);
            buffer_free(&buffer);
            pool_iterator_next(it);
        }
        pool_flush(database->table_pool);
    }) FINALLY({
        scheme_free(schema);
        buffer_free(&buffer);
        pool_iterator_free(&it);
    })
}

/// THROWS: [MALLOC_EXCEPTION, POOL_EXCEPTION]
static table_t database_find_table(database_t database, const char *const name) {
    assert(database != NULL && name != NULL);
    pool_it it = NULL;
    table_t table = NULL;
    buffer_t buffer = NULL;
    table_scheme *scheme = NULL;
    TRY({
        it = pool_iterator(database->table_pool);
        while (!pool_iterator_is_empty(it)) {
            buffer = pool_iterator_get(it);
            assert(NULL != buffer);
            scheme = table_schema_deserialize(buffer);
            if (0 == strcmp(scheme->name, name)) {
                pool_iterator_free(&it);
                table = rmalloc(sizeof(struct table));
                table->scheme = scheme;
                table->data_pool = pool_init(database->allocator, scheme->pool_offset);
                break;
            }
            scheme_free(scheme);
            buffer_free(&buffer);
            pool_iterator_next(it);
        }
    }) FINALLY({
        scheme_free(scheme);
        buffer_free(&buffer);
        pool_iterator_free(&it);
    })
    return table;
}

void database_insert(database_t database, const char *const name, row_value row) {
    assert(database != NULL && name != NULL && row != NULL);
    table_t table = NULL;
    buffer_t buffer = NULL;
    TRY({
        table = database_find_table(database, name);
        buffer = row_serialize(table->scheme, row);
        pool_append(table->data_pool, buffer);
        pool_flush(table->data_pool);
    }) FINALLY({
        buffer_free(&buffer);
        table_free(&table);
    })
}
