//
// Created by vyach on 02.10.2023.
//

#include <assert.h>
#include <stddef.h>
#include <string.h>
#include "cursor.h"
#include "util/exceptions/exceptions.h"

// THROWS: [MALLOC_EXCEPTION]
cursor_t cursor_init_from(table_t table, char *alias) {
    cursor_t cur = rmalloc(sizeof(struct cursor));
    cur->from.table = table;
    cur->from.it = pool_iterator(table->data_pool);
    cur->from.alias = alias;
    return cur;
}

void cursor_init_join_inner(cursor_t cur) {
    if (!join_condition_check(cur->join.info, cursor_get(cur))) {
        cursor_next(cur);
    }
}

void cursor_init_join_left(cursor_t cur) {
    cur->join.left_join.found = false;
    cur->join.left_join.val1 = NULL;
    cur->join.left_join.val2 = NULL;

}

// THROWS: [MALLOC_EXCEPTION]
cursor_t cursor_init_join(cursor_t left, cursor_t right, join_info info) {
    cursor_t cur = rmalloc(sizeof(struct cursor));
    cur->join.info = info;
    cur->join.right = right;
    cur->join.left = left;
    switch (info.type) {
        case JOIN_TYPE_INNER:
            cursor_init_join_inner(cur);
            break;
        case JOIN_TYPE_RIGHT:
            cur->join.left = right;
            cur->join.right = left;
        case JOIN_TYPE_LEFT:
            cursor_init_join_left(cur);
    }
    return cur;
}

// THROWS: [MALLOC_EXCEPTION]
cursor_t cursor_init_where(cursor_t base, where_condition condition) {
    cursor_t cur = rmalloc(sizeof(struct cursor));
    cur->where.condition = condition;
    cur->where.base = base;
    return cur;
}

bool cursor_is_empty(cursor_t cur) {
    assert(cur != NULL);
    switch (cur->type) {
        case CURSOR_FROM: {
            return pool_iterator_is_empty(cur->from.it);
        }
        case CURSOR_JOIN: {
            switch (cur->join.info.type) {
                case JOIN_TYPE_INNER:
                    return cursor_is_empty(cur->join.left) || cursor_is_empty(cur->join.right);
                case JOIN_TYPE_LEFT:
                case JOIN_TYPE_RIGHT:
                    return cursor_is_empty(cur->join.left);
            }
        }
        case CURSOR_WHERE: {
            return cursor_is_empty(cur);
        }
    }
    assert(false);
    return 0;
}

void cursor_delete(cursor_t cur, char *alias) {
    assert(cur != NULL);
    switch (cur->type) {
        case CURSOR_FROM: {
            if (0 == strcmp(alias, cur->from.alias)) {
                pool_iterator_delete(cur->from.it);
            }
            return;
        }
        case CURSOR_JOIN: {
            cursor_delete(cur->join.left, alias);
            cursor_delete(cur->join.right, alias);
            return;
        }
        case CURSOR_WHERE: {
            cursor_delete(cur->where.base, alias);
            return;
        }
    }
}

row_set_t cursor_get(cursor_t cur) {
    assert(cur != NULL);
    switch (cur->type) {
        case CURSOR_FROM: {
            table_scheme *scheme = cur->from.table->scheme;
            row_value row = row_deserialize(scheme, pool_iterator_get(cur->from.it));
            return row_set_from(cur->from.alias, row, scheme);
        }
        case CURSOR_JOIN: {
            row_set_t right_set = cursor_get(cur->join.right);
            row_set_t left_set = cursor_get(cur->join.left);
            if (right_set == NULL || left_set == NULL) {
                MAP_FREE(right_set);
                MAP_FREE(left_set);
                return NULL;
            }
            return row_set_merge(&right_set, &left_set);
        }
        case CURSOR_WHERE: {
            return cursor_get(cur->where.base);
        }
    }
    return NULL;
}

static void cursor_restart_from(cursor_t cur) {
    pool_iterator_free(&(cur->from.it));
    cur->from.it = pool_iterator(cur->from.table->data_pool);
}

static void cursor_restart_join_inner(cursor_t cur) {
    cursor_restart(cur->join.left);
    cursor_restart(cur->join.right);
    if (!join_condition_check(cur->join.info, cursor_get(cur))) {
        cursor_next(cur);
    }
}

bool cursor_join_left_find_next(cursor_t cur) {
    while (!cursor_is_empty(cur->join.left)) {
        cur->join.left_join.val2 = cursor_get(cur->join.left);
        if (join_condition_check(cur->join.info, cursor_get(cur))) {
            return true;
        }
        cursor_next(cur->join.right);
    }
    return false;
}

static void cursor_restart_join_left(cursor_t cur) {
    cur->join.left_join.found = false;
    MAP_FREE(cur->join.left_join.val1);
    MAP_FREE(cur->join.left_join.val2);
    if (!cursor_is_empty(cur->join.left)) {
        cur->join.left_join.val1 = cursor_get(cur->join.left);
        if (cursor_join_left_find_next(cur)) {
            return;
        }
        MAP_FREE(cur->join.left_join.val2);
    }
}

static void cursor_restart_join(cursor_t cur) {
    switch (cur->join.info.type) {
        case JOIN_TYPE_INNER:
            cursor_restart_join_inner(cur);
        case JOIN_TYPE_LEFT:
        case JOIN_TYPE_RIGHT:
            cursor_restart_join_left(cur);
    }
}

static void cursor_restart_where(cursor_t cur) {
    cursor_restart(cur->where.base);
    while (!where_condition_check(cur->where.condition, cursor_get(cur))) {
        cursor_next(cur->where.base);
    }
}

void cursor_restart(cursor_t cur) {
    assert(cur != NULL);
    switch (cur->type) {
        case CURSOR_FROM: {
            cursor_restart_from(cur);
            return;
        }
        case CURSOR_JOIN: {
            cursor_restart_join(cur);
            return;
        }
        case CURSOR_WHERE: {
            cursor_restart_where(cur);
            return;
        }
    }
}

static void cursor_next_from(cursor_t cur) {
    pool_iterator_next(cur->from.it);
}

static void cursor_next_join_inner(cursor_t cur) {
    if (cursor_is_empty(cur)) {
        return;
    }
    cursor_next(cur->join.right);
    if (cursor_is_empty(cur->join.right)) {
        cursor_next(cur->join.left);
        cursor_restart(cur->join.right);
    }
    while (!cursor_is_empty(cur->join.left) && !cursor_is_empty(cur->join.right)) {
        if (join_condition_check(cur->join.info, cursor_get(cur))) {
            break;
        }
        cursor_next(cur->join.right);
        if (cursor_is_empty(cur->join.right)) {
            cursor_next(cur->join.left);
            cursor_restart(cur->join.right);
        }
    }
}

static void cursor_next_join_left(cursor_t cur) {
    if (cursor_is_empty(cur)) {
        return;
    }
    if (cursor_is_empty(cur->join.right)) {
        cursor_next(cur->join.left);
        if (cursor_is_empty(cur)) {
            return;
        }
        cursor_restart(cur->join.right);
        MAP_FREE(cur->join.left_join.val1);
        cur->join.left_join.val1 = cursor_get(cur->join.left);
        if (cursor_join_left_find_next(cur)) {
            return;
        }
        MAP_FREE(cur->join.left_join.val2);
    } else {
        cursor_next(cur->join.right);
        if (!cursor_join_left_find_next(cur)) {
            cursor_next(cur);
        }
    }
}

static void cursor_next_join(cursor_t cur) {
    switch (cur->join.info.type) {
        case JOIN_TYPE_INNER:
            cursor_next_join_inner(cur);
        case JOIN_TYPE_LEFT:
        case JOIN_TYPE_RIGHT:
            cursor_next_join_left(cur);
    }
}

static void cursor_next_where(cursor_t cur) {
    cursor_t base = cur->where.base;
    cursor_next(base);
    while (!cursor_is_empty(base) &&
           !where_condition_check(cur->where.condition, cursor_get(cur))) {
        cursor_next(base);
    }
}

void cursor_next(cursor_t cur) {
    assert(cur != NULL);
    switch (cur->type) {
        case CURSOR_FROM: {
            cursor_next_from(cur);
            return;
        }
        case CURSOR_JOIN: {
            cursor_next_join(cur);
            return;
        }
        case CURSOR_WHERE: {
            cursor_next_where(cur);
            return;
        }
    }
}

// TODO: Implement
// void cursor_update(cursor_t cur, char *alias, updater_t *updater);
