//
// Created by vyach on 04.10.2023.
//

#ifndef LLP_LAB1_MAP_IMPL_H
#define LLP_LAB1_MAP_IMPL_H

#include "map.h"

size_t int_hash(const int *i);

size_t str_hash(const char *s);

bool int_equals(const int *i1, const int *i2);

bool str_equals(const char *s1, const char *s2);

int *int_copy(const int *i);

char *str_copy(const char *s);

void int_free(int *i);

void str_free(char *s);

// INT_STR
DECLARE_MAP(int, char*, int_str_map_t)
#define MAP_NEW_INT_STR(C) MAP_NEW(int_str_map_t, C, int_hash, int_equals, int_copy, int_free, str_copy, str_free)

// INT_INT
DECLARE_MAP(int, int, int_int_map_t)
#define MAP_NEW_INT_INT(C) MAP_NEW(int_int_map_t, C, int_hash, int_equals, int_copy, int_free, int_copy, int_free)

// STR_STR
DECLARE_MAP(char*, char*, str_str_map_t)
#define MAP_NEW_STR_STR(C) MAP_NEW(str_str_map_t, C, str_hash, str_equals, str_copy, str_free, str_copy, str_free)

#endif //LLP_LAB1_MAP_IMPL_H
