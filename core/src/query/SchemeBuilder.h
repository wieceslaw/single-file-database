//
// Created by vyach on 11.10.2023.
//

#ifndef LLP_LAB1_SCHEME_BUILDER_H
#define LLP_LAB1_SCHEME_BUILDER_H

#include "database/table/table.h"
#include "list/List.h"

typedef struct SchemeBuilder {
    char *name;
    List column_list;
} *SchemeBuilder;

SchemeBuilder SchemeBuilderNew(char *tableName);

void SchemeBuilderFree(SchemeBuilder builder);

int SchemeBuilderAddColumn(SchemeBuilder builder, char *name, ColumnType type);

table_scheme *SchemeBuilderBuild(SchemeBuilder builder);

#endif //LLP_LAB1_SCHEME_BUILDER_H
