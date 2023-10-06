//
// Created by vyach on 02.10.2023.
//

#include <assert.h>
#include <stddef.h>
#include <string.h>
#include "cursor.h"

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
}

void cursor_delete(cursor_t cur, char *alias) {
    assert(cur != NULL);
    switch (cur->type) {
        case CURSOR_FROM: {
            if (0 == strcmp(alias, cur->from.alias)) {
                pool_iterator_delete(cur->from.it);
            }
        }
        case CURSOR_JOIN: {
            cursor_delete(cur->join.left, alias);
            cursor_delete(cur->join.right, alias);
        }
        case CURSOR_WHERE: {
            cursor_delete(cur->where.base, alias);
        }
    }
}

row_set_t cursor_get(cursor_t cur) {
    assert(cur != NULL);
    switch (cur->type) {
        case CURSOR_FROM: {
            row_t row = row_deserialize(cur->from.table->scheme, pool_iterator_get(cur->from.it));
            return row_set_from(cur->from.alias, row);
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
}

void cursor_restart(cursor_t cur) {
    assert(cur != NULL);
    switch (cur->type) {
        case CURSOR_FROM: {
            pool_iterator_restart(cur->from.it);
            return;
        }
        case CURSOR_JOIN: {
            // TODO: implement
            return;
        }
        case CURSOR_WHERE: {
            // TODO: implement
            return;
        }
    }
}

void cursor_next(cursor_t cur) {
    assert(cur != NULL);
    switch (cur->type) {
        case CURSOR_FROM: {
            pool_iterator_next(cur->from.it);
            return;
        }
        case CURSOR_JOIN: {
            // TODO: implement
            return;
        }
        case CURSOR_WHERE: {
            // TODO: implement
            return;
        }
    }
}

// void cursor_update(cursor_t cur, char *alias, updater_t *updater);
