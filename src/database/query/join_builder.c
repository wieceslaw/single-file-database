//
// Created by vyach on 06.10.2023.
//

#include <assert.h>
#include "join_builder.h"
#include "database/table/table.h"
#include "util/exceptions/exceptions.h"
#include "util/list/list.h"

/// THROWS: [MALLOC_EXCEPTION]
join_builder_t join_builder_init() {
    join_builder_t set = rmalloc(sizeof(struct join_builder));
    set->join_condition_list = list_init();
    return set;
}

void join_builder_free(join_builder_t *set_ptr) {
    assert(set_ptr != NULL);
    join_builder_t set = *set_ptr;
    if (NULL == set) {
        return;
    }
    FOR_LIST(set->join_condition_list, it, {
        free(list_it_get(it));
    })
    list_free(&set->join_condition_list);
    free(set);
    *set_ptr = NULL;
}

/// THROWS: [MALLOC_EXCEPTION]
void join_builder_add(join_builder_t set, column_description left, column_description right) {
    assert(set != NULL);
    join_condition *condition = rmalloc(sizeof(join_condition));
    condition->right = right;
    condition->left = left;
    list_append_tail(set->join_condition_list, condition);
}
