//
// Created by vyach on 06.10.2023.
//

#ifndef LLP_LAB1_ROW_SET_H
#define LLP_LAB1_ROW_SET_H

#include "util/map/map.h"
#include "database/table/row.h"

typedef struct selector *selector_t;

// STR_ROW doesn't own data
DECLARE_MAP(char*, row_t, row_set_t)
#define MAP_NEW_STR_ROW(C) MAP_NEW(row_set_t, C, str_hash, str_equals, skip_copy, skip_free, skip_copy, skip_free)

// create set with single row
/// THROWS: [MALLOC_EXCEPTION]
row_set_t row_set_from(char* alias, row_t row);

// Will nullify both pointers and free one of sets
row_set_t row_set_merge(row_set_t *first, row_set_t *second);

// Will modify set
/// THROWS: [MALLOC_EXCEPTION]
row_t row_set_extract(row_set_t set, selector_t selector);

#endif //LLP_LAB1_ROW_SET_H
