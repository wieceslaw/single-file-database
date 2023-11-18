//
// Created by vyach on 09.10.2023.
//

#ifndef LLP_LAB1_SET_H
#define LLP_LAB1_SET_H

#define SET_NEW(T, CAPACITY, HASH_F, EQUALS_F, KEY_COPY, KEY_FREE)        \
    ((T)map_init(CAPACITY, HASH_F, EQUALS_F, KEY_COPY, KEY_FREE, skip_copy, skip_free)) \

#define SET_FREE(m) MAP_FREE(m)

#define SET_IS_EMPTY(m) MAP_IS_EMPTY(m)

#define SET_SIZE(m) MAP_SIZE(m)

// THROWS: [MALLOC_EXCEPTION]
#define SET_PUT(m, k, v) MAP_PUT(m, k, NULL)

#define SET_EXISTS(m, k) MAP_EXISTS(m, k)

#define SET_REMOVE(m, k) MAP_REMOVE(m, k)

#endif //LLP_LAB1_SET_H
