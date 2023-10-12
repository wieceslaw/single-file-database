//
// Created by vyach on 11.10.2023.
//

#ifndef LLP_LAB1_SELECTOR_BUILDER_H
#define LLP_LAB1_SELECTOR_BUILDER_H

#include "util/list/list.h"
#include "database/table/table.h"

typedef struct selector_builder {
    list_t columns_list;
} *selector_builder;

/// THROWS: [MALLOC_EXCEPTION]
selector_builder selector_builder_init();

void selector_builder_free(selector_builder *selector);

/// THROWS: [MALLOC_EXCEPTION]
void selector_builder_add(selector_builder selector, char* table_name, char* column_name);

#endif //LLP_LAB1_SELECTOR_BUILDER_H
