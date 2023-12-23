//
// Created by vyach on 06.10.2023.
//

#include <assert.h>
#include <malloc.h>
#include "database/table/table.h"
#include "JoinBuilder.h"

JoinBuilder JoinBuilderNew(void) {
    JoinBuilder builder = malloc(sizeof(struct JoinBuilder));
    if (builder == NULL) {
        return NULL;
    }
    builder->conditions = ListNew();
    return builder;
}

void JoinBuilderFree(JoinBuilder builder) {
    if (NULL == builder) {
        return;
    }
    ListApply(builder->conditions, free);
    ListFree(builder->conditions);
    builder->conditions = NULL;
    free(builder);
}

int JoinBuilderAddCondition(JoinBuilder builder, column_description left, column_description right) {
    assert(builder != NULL);
    join_condition *condition = malloc(sizeof(join_condition));
    if (condition == NULL) {
        return -1;
    }
    condition->right = right;
    condition->left = left;
    ListAppendTail(builder->conditions, condition);
    return 0;
}
