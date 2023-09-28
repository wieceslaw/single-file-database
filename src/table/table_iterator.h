//
// Created by vyach on 16.09.2023.
//

#ifndef LLP_LAB1_TABLE_IT_H
#define LLP_LAB1_TABLE_IT_H

#include <stdbool.h>
#include "allocator/buffer.h"
#include "table.h"

table_result table_iterator_free(table_it *it);

bool table_iterator_is_empty(table_it *it);

table_result table_iterator_next(table_it *it);

table_result table_iterator_restart(table_it *it);

table_result table_iterator_get(table_it *it, buffer_t *data);

table_result table_iterator_update(table_it *it, buffer_t *data);

table_result table_iterator_delete(table_it *it);

#endif //LLP_LAB1_TABLE_IT_H
