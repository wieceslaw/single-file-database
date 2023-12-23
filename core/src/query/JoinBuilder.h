
//
// Created by vyach on 06.10.2023.
//

#ifndef LLP_LAB1_JOIN_BUILDER_H
#define LLP_LAB1_JOIN_BUILDER_H

#include <stdbool.h>
#include "database/table/table.h"
#include "list/List.h"

typedef struct join_condition {
    column_description left;
    column_description right;
} join_condition;

typedef struct JoinBuilder {
    List conditions;
} *JoinBuilder;

JoinBuilder JoinBuilderNew(void);

void JoinBuilderFree(JoinBuilder builder);

int JoinBuilderAddCondition(JoinBuilder builder, column_description left, column_description right);

#endif //LLP_LAB1_JOIN_BUILDER_H
