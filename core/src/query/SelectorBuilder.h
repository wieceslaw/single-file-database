//
// Created by vyach on 11.10.2023.
//

#ifndef LLP_LAB1_SELECTOR_BUILDER_H
#define LLP_LAB1_SELECTOR_BUILDER_H

#include "list/list.h"
#include "database/table/table.h"

typedef struct SelectorBuilder {
    list_t columns;
} *SelectorBuilder;

SelectorBuilder SelectorBuilderNew(void);

void SelectorBuilderFree(SelectorBuilder selector);

int SelectorBuilderAdd(SelectorBuilder selector, char *tableName, char *columnName);

#endif //LLP_LAB1_SELECTOR_BUILDER_H
