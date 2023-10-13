//
// Created by vyach on 04.10.2023.
//

#ifndef LLP_LAB1_MAP_IMPL_H
#define LLP_LAB1_MAP_IMPL_H

#include "map.h"
#include "database/table/table.h"

size_t int_hash(const void *x);

size_t str_hash(const void *p);

bool int_equals(const void *x1, const void *x2);

bool str_equals(const void *x1, const void *x2);

void *int_copy(const void *x);

void *str_copy(const void *x);

void int_free(void *x);

void str_free(void *x);

void skip_free(void *x);

void* skip_copy(const void *x);

// INT, STR
DECLARE_MAP(int*, char*, int_str_map_t)
#define MAP_NEW_INT_STR(C) MAP_NEW(int_str_map_t, C, int_hash, int_equals, int_copy, int_free, str_copy, str_free)

// INT, INT
DECLARE_MAP(int*, int*, int_int_map_t)
#define MAP_NEW_INT_INT(C) MAP_NEW(int_int_map_t, C, int_hash, int_equals, int_copy, int_free, int_copy, int_free)

// STR, STR
DECLARE_MAP(char*, char*, str_str_map_t)
#define MAP_NEW_STR_STR(C) MAP_NEW(str_str_map_t, C, str_hash, str_equals, str_copy, str_free, str_copy, str_free)

// STR, INT
DECLARE_MAP(char*, int*, str_int_map_t)
#define MAP_NEW_STR_INT(C) MAP_NEW(str_int_map_t, C, str_hash, str_equals, str_copy, str_free, int_copy, int_free)

// STR, MAP[STR, INT]
DECLARE_MAP(char*, str_int_map_t, str_map_str_int_map_t)
#define MAP_NEW_STR_MAP_STR_INT(C) MAP_NEW(str_map_str_int_map_t, C, str_hash, str_equals, str_copy, str_free, skip_copy, skip_free)

// STR, VOID
DECLARE_MAP(char*, void*, str_void_map_t)
#define MAP_NEW_STR_VOID(C) MAP_NEW(str_void_map_t, C, str_hash, str_equals, str_copy, str_free, skip_copy, skip_free)

#endif //LLP_LAB1_MAP_IMPL_H
