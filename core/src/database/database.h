//
// Created by vyach on 02.10.2023.
//

#ifndef LLP_LAB1_DATABASE_H
#define LLP_LAB1_DATABASE_H

#include "allocator/allocator.h"
#include "map/map_impl.h"
#include "table/table.h"
#include "database/query/where_condition.h"
#include "database/query/join_builder.h"
#include "database/query/updater_builder.h"
#include "database/query/result_view.h"
#include "database/query/selector_builder.h"
#include "database/query/row_batch.h"
#include "database/query/scheme_builder.h"

typedef struct indexed_maps {
    str_map_str_int_map_t columns_maps;
    str_int_map_t table_maps;
} indexed_maps;

typedef struct query {
    char *table;
    join_builder_t joins;
    where_condition *where;
} query_t;

typedef struct database *database_t;

void indexed_maps_free(indexed_maps maps);

column_description indexed_maps_translate(indexed_maps maps, column_description description);

where_condition *where_condition_translate(where_condition *condition, indexed_maps maps);

/// THROWS: [DATABASE_INTERNAL_ERROR]
database_t database_init(file_settings *settings);

/// THROWS: [DATABASE_INTERNAL_ERROR]
void database_free(database_t database);

/// THROWS: [DATABASE_TABLE_ALREADY_EXISTS, DATABASE_INTERNAL_ERROR]
void database_create_table(database_t database, scheme_builder_t scheme_builder);

/// THROWS: [DATABASE_INTERNAL_ERROR]
void database_delete_table(database_t database, char *table_name);

/// THROWS: [DATABASE_INTERNAL_ERROR, DATABASE_QUERY_EXCEPTION]
void database_insert(database_t database, char * name, batch_builder_t batch);

/// THROWS: [DATABASE_INTERNAL_ERROR, DATABASE_QUERY_EXCEPTION]
void database_delete(database_t database, query_t query);

/// THROWS: [DATABASE_INTERNAL_ERROR, DATABASE_QUERY_EXCEPTION]
void database_update(database_t database, query_t query, updater_builder_t updater);

/// THROWS: [DATABASE_INTERNAL_ERROR, DATABASE_QUERY_EXCEPTION]
result_view_t database_select(database_t database, query_t query, selector_builder selector);

#endif //LLP_LAB1_DATABASE_H
