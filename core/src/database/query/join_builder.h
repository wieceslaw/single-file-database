
//
// Created by vyach on 06.10.2023.
//

#ifndef LLP_LAB1_JOIN_BUILDER_H
#define LLP_LAB1_JOIN_BUILDER_H

#include <stdbool.h>
#include "database/table/table.h"
#include "list/list.h"

typedef struct join_condition {
    column_description left;
    column_description right;
} join_condition;

typedef struct join_builder {
    list_t join_condition_list;
} *join_builder_t;

/// THROWS: [MALLOC_EXCEPTION]
join_builder_t join_builder_init();

void join_builder_free(join_builder_t *set);

/// THROWS: [MALLOC_EXCEPTION]
void join_builder_add(join_builder_t set, column_description left, column_description right);

#endif //LLP_LAB1_JOIN_BUILDER_H
