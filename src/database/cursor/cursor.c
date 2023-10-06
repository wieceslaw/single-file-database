//
// Created by vyach on 02.10.2023.
//

#include <assert.h>
#include <stddef.h>
#include <string.h>
#include "cursor.h"
#include "util/exceptions/exceptions.h"

// THROWS: [MALLOC_EXCEPTION]
cursor_t cursor_type_from(table_t table, char* alias) {
    cursor_t cur = rmalloc(sizeof(struct cursor));
    cur->from.table = table;
    cur->from.it = pool_iterator(table->data_pool);
    cur->from.alias = alias;
    return cur;
}

// THROWS: [MALLOC_EXCEPTION]
cursor_t cursor_type_join(cursor_t left, cursor_t right, join_condition condition, join_type type) {
    // TODO: Implement
    return NULL;
}

// THROWS: [MALLOC_EXCEPTION]
cursor_t cursor_type_where(cursor_t base, where_condition condition) {
    // TODO: Implement
    return NULL;
}

bool cursor_is_empty(cursor_t cur) {
    assert(cur != NULL);
    switch (cur->type) {
        case CURSOR_FROM: {
            return pool_iterator_is_empty(cur->from.it);
        }
        case CURSOR_JOIN: {
            switch (cur->join.type) {
                case JOIN_TYPE_INNER:
                    return cursor_is_empty(cur->join.left) || cursor_is_empty(cur->join.right);
                case JOIN_TYPE_LEFT:
                    return cursor_is_empty(cur->join.left);
                case JOIN_TYPE_RIGHT:
                    return cursor_is_empty(cur->join.right);
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

static void cursor_restart_join(cursor_t cur) {
    // TODO: Implement
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

void cursor_next_from(cursor_t cur) {
    pool_iterator_next(cur->from.it);
}

void cursor_next_join(cursor_t cur) {
    // TODO: Implement
}

void cursor_next_where(cursor_t cur) {
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
