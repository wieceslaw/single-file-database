//
// Created by vyach on 02.10.2023.
//

#ifndef LLP_LAB1_DATABASE_H
#define LLP_LAB1_DATABASE_H

#include "allocator/allocator.h"
#include "table/table.h"
#include "database/table/row.h"

typedef enum {
    DATABASE_OP_OK = 0,
    DATABASE_OP_ERR = 1,
} database_result;

typedef struct database database_t;

database_t *database_init(file_settings *settings);

database_result database_free(database_t *database);

database_result database_create_table(database_t *database, scheme_t *table_schema);

database_result database_delete_table(database_t *database, const char *name);

table_result_type database_insert(database_t *database, const char *name, const row_t *row);

// TODO: implement add (join, where)
// selector
// updater
//result_set_t database_select(database_t *database, table_alias_t *table);
//
//table_result_type database_delete(database_t *database, table_alias_t *table);
//
//table_result_type database_update(database_t *database, table_alias_t *table);

#endif //LLP_LAB1_DATABASE_H
