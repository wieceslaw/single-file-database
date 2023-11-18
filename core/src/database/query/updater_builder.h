//
// Created by vyach on 11.10.2023.
//

#ifndef LLP_LAB1_UPDATER_BUILDER_H
#define LLP_LAB1_UPDATER_BUILDER_H

#include "database/table/table.h"
#include "list/list.h"
#include "map/map_impl.h"

typedef struct column_updater {
    bool translated;
    union {
        size_t idx;
        char* name;
    } target;
    column_t new_value;
} column_updater;

typedef struct updater_builder {
    list_t column_updaters;
} *updater_builder_t;

/// THROWS: [MALLOC_EXCEPTION]
column_updater *column_updater_of(char* target, column_t new_value);

/// THROWS: [MALLOC_EXCEPTION]
updater_builder_t updater_builder_init();

updater_builder_t updater_builder_translate(updater_builder_t old_updater, str_int_map_t map);

void updater_builder_free(updater_builder_t* updater_ptr);

/// THROWS: [MALLOC_EXCEPTION]
void updater_builder_add(updater_builder_t updater, column_updater *);

row_t updater_builder_update(updater_builder_t updater, row_t row);

#endif //LLP_LAB1_UPDATER_BUILDER_H
