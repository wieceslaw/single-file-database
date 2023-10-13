//
// Created by vyach on 02.10.2023.
//

#ifndef LLP_LAB1_DATABASE_H
#define LLP_LAB1_DATABASE_H

#include "allocator/allocator.h"
#include "util/map/map_impl.h"
#include "table/table.h"
#include "database/query/where_condition.h"
#include "database/query/join_builder.h"
#include "database/query/updater_builder.h"
#include "database/query/result_view.h"
#include "database/query/selector_builder.h"
#include "database/query/row_batch.h"

typedef enum database_result {
    DATABASE_RESULT_SUCCESS = 0,
    DATABASE_RESULT_INTERNAL_ERROR = 1,
    DATABASE_RESULT_WRONG_QUERY = 2,
} database_result;

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

database_t database_init(file_settings *settings);

void database_free(database_t database);

void database_create_table(database_t database, table_scheme *table_scheme);

void database_delete_table(database_t database, char *table_name);

database_result database_insert(database_t database, char * name, batch_builder_t batch);

database_result database_delete(database_t database, query_t query);

database_result database_update(database_t database, query_t query, updater_builder_t updater);

result_view_t database_select(database_t database, query_t query, selector_builder selector);

#endif //LLP_LAB1_DATABASE_H
