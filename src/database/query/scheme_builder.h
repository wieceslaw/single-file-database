//
// Created by vyach on 11.10.2023.
//

#ifndef LLP_LAB1_SCHEME_BUILDER_H
#define LLP_LAB1_SCHEME_BUILDER_H

#include "database/table/table.h"

typedef struct scheme_builder *scheme_builder_t;

/// THROWS: [MALLOC_EXCEPTION]
scheme_builder_t scheme_builder_init(char *name);

void scheme_builder_free(scheme_builder_t *builder);

/// THROWS: [MALLOC_EXCEPTION]
void scheme_builder_add_column(scheme_builder_t builder, char *name, column_type type);

/// THROWS: [MALLOC_EXCEPTION]
table_scheme *scheme_builder_build(scheme_builder_t builder);

#endif //LLP_LAB1_SCHEME_BUILDER_H
