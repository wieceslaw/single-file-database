//
// Created by vyach on 02.10.2023.
//

#include <stddef.h>
#include <assert.h>
#include <string.h>
#include "allocator/allocator.h"
#include "pool/pool.h"
#include "table/table.h"
#include "database.h"
#include "util/exceptions/exceptions.h"
#include "database/cursor/cursor.h"
#include "util/map/map_impl.h"
#include "database/query/scheme_builder.h"
#include "database/query/row_batch.h"
#include "util/string.h"

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
    buffer_reset(buffer);
    schema = rmalloc(sizeof(table_scheme));
    schema->pool_offset = buffer_read_b64(buffer).ui64;
    schema->name = buffer_read_string(buffer);
    schema->size = buffer_read_b32(buffer).ui32;
    schema->columns = rmalloc(sizeof(table_scheme_column) * schema->size);
    read_columns(schema, buffer);
    TRY({
        //
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

/// THROWS: [MALLOC_EXCEPTION]
database_t database_init(file_settings *settings) {
    database_t database;
    TRY({
        database = rmalloc(sizeof(struct database));
        allocator_t *allocator;
        if (allocator_init(settings, &allocator) != FILE_ST_OK) {
            RAISE(DATABASE_INTERNAL_ERROR);
        }
        if (settings->open_mode == FILE_OPEN_CLEAR || settings->open_mode == FILE_OPEN_CREATE) {
            offset_t offset = pool_create(allocator);
            allocator_set_entrypoint(allocator, offset);
        }
        pool_t *table_pool = pool_init(allocator, allocator_get_entrypoint(allocator));
        database->table_pool = table_pool;
        database->allocator = allocator;
    }) CATCH(exception >= EXCEPTION, {
        free(database);
        RAISE(DATABASE_INTERNAL_ERROR);
    }) FINALLY()
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
        RAISE(DATABASE_INTERNAL_ERROR);
    }) FINALLY()
}

void database_create_table(database_t database, scheme_builder_t scheme_builder) {
    assert(database != NULL && scheme_builder != NULL);
    pool_it it = NULL;
    table_scheme *table_schema = NULL;
    TRY({
        table_schema = scheme_builder_build(scheme_builder);
        it = pool_iterator(database->table_pool);
        while (!pool_iterator_is_empty(it)) {
            buffer_t buffer = pool_iterator_get(it);
            assert(NULL != buffer);
            table_scheme *schema = table_schema_deserialize(buffer);
            if (0 == strcmp(schema->name, table_schema->name)) {
                pool_iterator_free(&it);
                RAISE(DATABASE_TABLE_ALREADY_EXISTS);
            }
            table_scheme_free(schema);
            buffer_free(&buffer);
            pool_iterator_next(it);
        }
    }) CATCH(exception >= EXCEPTION, {
        RAISE(DATABASE_INTERNAL_ERROR);
    }) FINALLY({
        pool_iterator_free(&it);
    })
    buffer_t serialized;
    TRY({
        offset_t offset = pool_create(database->allocator);
        table_schema->pool_offset = offset;
        serialized = table_serialize(table_schema);
        pool_append(database->table_pool, serialized);
        pool_flush(database->table_pool);
    }) CATCH(exception >= EXCEPTION, {
        RAISE(DATABASE_INTERNAL_ERROR);
    }) FINALLY({
        table_scheme_free(table_schema);
        buffer_free(&serialized);
    })
}

void database_delete_table(database_t database, char *name) {
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
            table_scheme_free(schema);
            buffer_free(&buffer);
            pool_iterator_next(it);
        }
        pool_flush(database->table_pool);
    }) CATCH(exception >= EXCEPTION, {
        RAISE(DATABASE_INTERNAL_ERROR);
    }) FINALLY({
        table_scheme_free(schema);
        buffer_free(&buffer);
        pool_iterator_free(&it);
    })
}

/// THROWS: [MALLOC_EXCEPTION, POOL_EXCEPTION]
static table_scheme *database_find_table_scheme(database_t database, const char *const name) {
    assert(database != NULL && name != NULL);
    pool_it it = NULL;
    buffer_t buffer = NULL;
    table_scheme *scheme = NULL;
    TRY({
        it = pool_iterator(database->table_pool);
        while (!pool_iterator_is_empty(it)) {
            buffer = pool_iterator_get(it);
            assert(NULL != buffer);
            scheme = table_schema_deserialize(buffer);
            if (0 == strcmp(scheme->name, name)) {
                break;
            }
            table_scheme_free(scheme);
            buffer_free(&buffer);
            pool_iterator_next(it);
        }
    }) FINALLY({
        buffer_free(&buffer);
        pool_iterator_free(&it);
    })
    return scheme;
}

/// THROWS: [MALLOC_EXCEPTION, POOL_EXCEPTION]
static table_t database_find_table(database_t database, char *name) {
    assert(database != NULL && name != NULL);
    table_t table = NULL;
    TRY({
        table = rmalloc(sizeof(struct table));
        table->scheme = database_find_table_scheme(database, name);
        if (table->scheme == NULL) {
            free(table);
            return NULL;
        }
        table->data_pool = pool_init(database->allocator, table->scheme->pool_offset);
    }) CATCH(exception >= EXCEPTION, {
        table_scheme_free(table->scheme);
        free(table);
        RAISE(exception);
    }) FINALLY()
    return table;
}

static bool row_is_valid(table_scheme *scheme, row_builder_t row) {
    for (size_t i = 0; i < row.size; i++) {
        if (scheme->columns[i].type != row.columns[i].type) {
            return false;
        }
    }
    return true;
}

static bool batch_is_valid(table_scheme *scheme, batch_builder_t batch) {
    for (size_t i = 0; i < batch.size; i++) {
        if (!row_is_valid(scheme, batch.rows[i])) {
            return false;
        }
    }
    return true;
}

void database_insert(database_t database, char * name, batch_builder_t batch) {
    assert(database != NULL && name != NULL);
    table_t table = NULL;
    buffer_t buffer = NULL;
    table = database_find_table(database, name);
    if (NULL == table) {
        RAISE(DATABASE_QUERY_EXCEPTION);
    }
    if (!batch_is_valid(table->scheme, batch)) {
        RAISE(DATABASE_QUERY_EXCEPTION);
    }
    TRY({
        for (size_t i = 0; i < batch.size; i++) {
            row_t row = row_builder_as_row(&(batch.rows[i]));
            buffer = row_serialize(row);
            pool_append(table->data_pool, buffer);
            buffer_free(&buffer);
        }
        pool_flush(table->data_pool);
        table_free(&table);
    }) CATCH(exception >= EXCEPTION, {
        RAISE(DATABASE_INTERNAL_ERROR);
    }) FINALLY()
}

static cursor_t database_build_base_cursor(database_t database, char* table_name, size_t table_idx) {
    cursor_t cur = NULL;
    TRY({
        table_t table = database_find_table(database, table_name);
        cur = cursor_init_from(table, table_idx);
    }) CATCH(exception >= EXCEPTION, {
        cursor_free(&cur);
        RAISE(exception);
    }) FINALLY()
    return cur;
}

/// THROWS: [MALLOC_EXCEPTION, DATABASE_TRANSLATION_EXCEPTION]
column_description indexed_maps_translate(indexed_maps maps, column_description description) {
    column_description result;
    int *column_idx = NULL;
    int *table_idx = NULL;
    TRY({
        table_idx = MAP_GET(maps.table_maps, description.name.table_name);
        if (table_idx == NULL) {
            RAISE(DATABASE_TRANSLATION_EXCEPTION);
        }
        str_int_map_t column_map = MAP_GET(maps.columns_maps, description.name.table_name);
        if (column_map == NULL) {
            RAISE(DATABASE_TRANSLATION_EXCEPTION);
        }
        column_idx = MAP_GET(column_map, description.name.column_name);
        if (column_idx == NULL) {
            RAISE(DATABASE_TRANSLATION_EXCEPTION);
        }
        result.type = COLUMN_DESC_INDEX;
        result.index.table_idx = *table_idx;
        result.index.column_idx = *column_idx;
    }) FINALLY({
       free(column_idx);
       free(table_idx);
    })
    return result;
}

/// THROWS: [MALLOC_EXCEPTION, POOL_EXCEPTION]
static str_int_map_t table_scheme_mapping(database_t database, char* table_name) {
    // MAP[column_name, column_idx]
    table_scheme *scheme = NULL;
    str_int_map_t map = NULL;
    TRY({
        scheme = database_find_table_scheme(database, table_name);
        if (NULL == scheme) {
            return NULL;
        }
        map = MAP_NEW_STR_INT(scheme->size);
        for (int i = 0; i < scheme->size; i++) {
            table_scheme_column col = scheme->columns[i];
            MAP_PUT(map, col.name, &i);
        }
    }) FINALLY({
        table_scheme_free(scheme);
    })
    return map;
}

/// THROWS: [TRANSLATION_EXCEPTION, EXCEPTION]
static indexed_maps query_mapping(database_t database, query_t query) {
    assert(query.table != NULL);
    size_t size = 1;
    if (query.joins != NULL) {
        size += list_size(query.joins->join_condition_list);
    }
    str_map_str_int_map_t columns_maps = MAP_NEW_STR_MAP_STR_INT(size);
    str_int_map_t table_maps = MAP_NEW_STR_INT(size);
    int count = 0;
    str_int_map_t columns_map = table_scheme_mapping(database, query.table);
    if (NULL == columns_map) {
        RAISE(DATABASE_TRANSLATION_EXCEPTION);
    }
    MAP_PUT(columns_maps, query.table, columns_map);
    MAP_PUT(table_maps, query.table, &count);
    if (query.joins != NULL) {
        FOR_LIST(query.joins->join_condition_list, it, {
            count++;
            join_condition *condition = list_it_get(it);
            assert(condition != NULL);
            assert(condition->left.type == COLUMN_DESC_NAME && condition->right.type == COLUMN_DESC_NAME);
            str_int_map_t left_columns_map = MAP_GET(columns_maps, condition->left.name.table_name);
            if (NULL == left_columns_map) { // table should exists in join set
                RAISE(DATABASE_TRANSLATION_EXCEPTION);
            }
            if (!MAP_EXISTS(left_columns_map, condition->left.name.column_name)) { // table should have such column
                RAISE(DATABASE_TRANSLATION_EXCEPTION);
            }
            if (MAP_EXISTS(columns_maps, condition->right.name.table_name)) { // can't self-join
                RAISE(DATABASE_TRANSLATION_EXCEPTION);
            }
            str_int_map_t right_columns_map = table_scheme_mapping(database, condition->right.name.table_name);
            if (NULL == right_columns_map) { // table should exist in database
                RAISE(DATABASE_TRANSLATION_EXCEPTION);
            }
            if (!MAP_EXISTS(right_columns_map, condition->right.name.column_name)) { // right table has such column
                RAISE(DATABASE_TRANSLATION_EXCEPTION);
            }
            MAP_PUT(columns_maps, condition->right.name.table_name, right_columns_map);
            MAP_PUT(table_maps, condition->right.name.table_name, &count);
        })
    }
    return (indexed_maps) {
        .columns_maps = columns_maps,
        .table_maps = table_maps
    };
}

void indexed_maps_free(indexed_maps maps) {
    if (maps.table_maps != NULL) {
        MAP_FREE(maps.table_maps);
    }
    if (maps.columns_maps != NULL) {
        FOR_MAP(maps.columns_maps, entry, {
            MAP_FREE(entry->val);
            free(entry->key);
            free(entry);
        })
        MAP_FREE(maps.columns_maps);
    }
}

/// THROWS: [DATABASE_TRANSLATION_EXCEPTION]
static cursor_t query_cursor(database_t database, query_t query, indexed_maps maps) {
    assert(database != NULL && query.table != NULL);
    cursor_t result = database_build_base_cursor(database, query.table, 0);
    if (query.joins != NULL) {
        FOR_LIST(query.joins->join_condition_list, it, {
            TRY({
                join_condition *condition = list_it_get(it);
                assert(condition != NULL);
                join_condition translated_condition;
                translated_condition.right = indexed_maps_translate(maps, condition->right);
                translated_condition.left = indexed_maps_translate(maps, condition->left);
                cursor_t right = database_build_base_cursor(
                        database,
                        condition->right.name.table_name,
                        translated_condition.right.index.table_idx
                );
                cursor_t joined = cursor_init_join(result, right, translated_condition);
                result = joined;
            }) CATCH(exception >= EXCEPTION, {
                list_it_free(&it);
                cursor_free(&result);
                indexed_maps_free(maps);
                RAISE(exception);
            }) FINALLY()
        })
    }
    if (query.where != NULL) {
        where_condition *translated_condition;
        TRY({
            translated_condition = where_condition_translate(query.where, maps);
            cursor_t tmp = cursor_init_where(result, translated_condition);
            result = tmp;
        }) CATCH(exception >= exception, {
            where_condition_free(translated_condition);
            cursor_free(&result);
            RAISE(exception);
        }) FINALLY()
    }
    return result;
}

void database_delete(database_t database, query_t query) {
    assert(database != NULL);
    indexed_maps maps = {0};
    cursor_t cur = NULL;
    TRY({
        maps = query_mapping(database, query);
        cur = query_cursor(database, query, maps);
        while (!cursor_is_empty(cur)) {
            cursor_delete(cur, 0);
            cursor_next(cur);
        }
        cursor_flush(cur);
        cursor_free(&cur);
    }) CATCH(exception == DATABASE_TRANSLATION_EXCEPTION, {
        RAISE(DATABASE_QUERY_EXCEPTION);
    }) CATCH (exception >= EXCEPTION, {
        RAISE(DATABASE_INTERNAL_ERROR);
    }) FINALLY({
        indexed_maps_free(maps);
    })
}

void database_update(database_t database, query_t query, updater_builder_t updater) {
    assert(database != NULL);
    indexed_maps maps = {0};
    cursor_t cur = NULL;
    str_int_map_t map = NULL;
    updater_builder_t translated_updater = NULL;
    TRY({
        maps = query_mapping(database, query);
        cur = query_cursor(database, query, maps);
        map = MAP_GET(maps.columns_maps, query.table);
        translated_updater = updater_builder_translate(updater, map);
        while (!cursor_is_empty(cur)) {
            cursor_update(cur, 0, translated_updater);
            cursor_next(cur);
        }
        cursor_flush(cur);
        cursor_free(&cur);
    }) CATCH(exception == DATABASE_TRANSLATION_EXCEPTION, {
        RAISE(DATABASE_QUERY_EXCEPTION);
    }) CATCH (exception >= EXCEPTION, {
        RAISE(DATABASE_INTERNAL_ERROR);
    }) FINALLY({
        indexed_maps_free(maps);
        updater_builder_free(&translated_updater);
    })
}

static table_scheme *create_view_scheme(database_t database, selector_builder selector, indexed_maps maps) {
    scheme_builder_t view_scheme_builder = scheme_builder_init("");
    FOR_LIST(selector->columns_list, it, {
        column_description *description = list_it_get(it);
        assert(description != NULL);
        column_description indexed_description = indexed_maps_translate(maps, *description);
        // add schemes cache?
        table_scheme *scheme = database_find_table_scheme(database, description->name.table_name);
        table_scheme_column column = scheme->columns[indexed_description.index.column_idx];
        scheme_builder_add_column(view_scheme_builder, column.name, column.type);
        table_scheme_free(scheme);
    })
    table_scheme *result = scheme_builder_build(view_scheme_builder);
    scheme_builder_free(&view_scheme_builder);
    return result;
}

static column_description *create_view_selector(selector_builder selector, indexed_maps maps) {
    column_description *result = rmalloc(sizeof(column_description) * list_size(selector->columns_list));
    int i = 0;
    FOR_LIST(selector->columns_list, it, {
        column_description *description = list_it_get(it);
        assert(description != NULL);
        result[i] = indexed_maps_translate(maps, *description);
        i++;
    })
    return result;
}

result_view_t database_select(database_t database, query_t query, selector_builder selector) {
    assert(database != NULL && selector != NULL);
    result_view_t result = NULL;
    indexed_maps maps = {0};
    cursor_t cur = NULL;
    TRY({
        result = rmalloc(sizeof(struct result_view));
        maps = query_mapping(database, query);
        cur = query_cursor(database, query, maps);
        result->cursor = cur;
        result->view_scheme = create_view_scheme(database, selector, maps);
        result->view_selector = create_view_selector(selector, maps);
        indexed_maps_free(maps);
    }) CATCH(exception == DATABASE_TRANSLATION_EXCEPTION, {
        free(result);
        indexed_maps_free(maps);
        RAISE(DATABASE_QUERY_EXCEPTION);
    }) CATCH(exception >= EXCEPTION, {
        free(result);
        indexed_maps_free(maps);
        RAISE(DATABASE_INTERNAL_ERROR);
    }) FINALLY()
    return result;
}

static operand column_operand_translate(operand op, indexed_maps maps) {
    switch (op.type) {
        case OPERAND_VALUE_LITERAL: {
            return op;
        }
        case OPERAND_VALUE_COLUMN: {
            return (operand) {
                    .type = OPERAND_VALUE_COLUMN,
                    .column = indexed_maps_translate(maps, op.column)
            };
        }
    }
}

static where_condition *where_condition_translate_indexed(where_condition *condition, indexed_maps maps) {
    assert(condition->type == CONDITION_COMPARE);
    where_condition *result = rmalloc(sizeof(where_condition));
    result->type = CONDITION_COMPARE;
    result->compare.type = condition->compare.type;
    result->compare.first = column_operand_translate(condition->compare.first, maps);
    result->compare.second = column_operand_translate(condition->compare.second, maps);
    return result;
}

where_condition *where_condition_translate(where_condition *condition, indexed_maps maps) {
    switch (condition->type) {
        case CONDITION_AND:
            return where_condition_and(
                    where_condition_translate(condition->and.first, maps),
                    where_condition_translate(condition->and.second, maps)
            );
        case CONDITION_OR:
            return where_condition_or(
                    where_condition_translate(condition->or.first, maps),
                    where_condition_translate(condition->or.second, maps)
            );
        case CONDITION_NOT:
            return where_condition_not(
                    where_condition_translate(condition->not.first, maps)
            );
        case CONDITION_COMPARE:
            return where_condition_translate_indexed(condition, maps);
    }
}
