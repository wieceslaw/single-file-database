//
// Created by vyach on 04.10.2023.
//

#ifndef LLP_LAB1_MAP_H
#define LLP_LAB1_MAP_H

#include <stddef.h>
#include <stdbool.h>
#include "util/defines.h"
#include "util/list/list.h"

#define MAP_START_CAPACITY 8
#define MAP_EXTENSION_RATIO 2
#define MAP_LOAD_FACTOR 0.75

#define FOR_MAP(M, E, CODE) { \
    map_it __it = map_get_iterator((map_t)M); \
    while (!map_it_is_empty(__it)) { \
        map_entry_t E = map_it_get_entry(__it); \
        CODE                      \
        map_it_next(__it); \
    } \
    map_it_free(&__it); \
} \

typedef size_t (*hash_f)(const void *);

typedef bool (*equals_f)(const void *, const void *);

typedef void *(*copy_f)(const void *);

typedef void (*free_f)(void *);

typedef void *map_key;

typedef void *map_value;

typedef struct map_entry {
    map_key key;
    map_value val;
} *map_entry_t;

typedef struct map_iterator *map_it;

#define MAP_PUT_SIGNATURE(M, K, V, name) void(*name)(M, K, V)

#define MAP_GET_SIGNATURE(M, K, V, name) V(*name)(M, K)

#define MAP_EXISTS_SIGNATURE(M, K, V, name) bool(*name)(M, K)

#define MAP_REMOVE_SIGNATURE(M, K, V, name) void(*name)(M, K)

typedef struct generic_map {
    MAP_PUT_SIGNATURE(struct generic_map*, map_key, map_value, put);

    MAP_GET_SIGNATURE(struct generic_map*, map_key, map_value, get);

    MAP_EXISTS_SIGNATURE(struct generic_map*, map_key, map_value, exists);

    MAP_REMOVE_SIGNATURE(struct generic_map*, map_key, map_value, remove);

    list_t *buckets;
    size_t capacity;
    size_t size;
    hash_f key_hash;
    equals_f key_equals;
    copy_f key_copy;
    free_f key_free;
    copy_f val_copy;
    free_f val_free;

} *map_t;

// THROWS: [MALLOC_EXCEPTION]
map_t map_init(size_t capacity, hash_f key_hash, equals_f key_equals,
               copy_f key_copy, free_f key_free, copy_f val_copy, free_f val_free);

void map_free(map_t *map);

size_t generic_map_size(map_t map);

bool map_is_empty(map_t map);

void map_remove(map_t map, map_key key);

// THROWS: [MALLOC_EXCEPTION]
void map_put(map_t map, map_key key, map_value value);

map_value map_get(map_t map, map_key key);

bool map_exists(map_t map, map_key key);

// THROWS: [MALLOC_EXCEPTION]
map_it map_get_iterator(map_t map);

void map_it_free(map_it *it);

bool map_it_is_empty(map_it it);

void map_it_next(map_it it);

// THROWS: [MALLOC_EXCEPTION]
map_entry_t map_it_get_entry(map_it it);

#define DECLARE_MAP(K, V, T) typedef struct T##_s {    \
    MAP_PUT_SIGNATURE(struct T##_s*, K, V, put);       \
                                                       \
    MAP_GET_SIGNATURE(struct T##_s*, K, V, get);       \
                                                       \
    MAP_EXISTS_SIGNATURE(struct T##_s*, K, V, exists); \
                                                       \
    MAP_REMOVE_SIGNATURE(struct T##_s*, K, V, remove); \
    } *T;                                              \

#define MAP_NEW(T, CAPACITY, HASH_F, EQUALS_F, KEY_COPY, KEY_FREE, VAL_COPY, VAL_FREE)        \
    ((T)map_init(CAPACITY, HASH_F, EQUALS_F, KEY_COPY, KEY_FREE, VAL_COPY, VAL_FREE)) \

#define MAP_FREE(m) (map_free((map_t*)&m))

#define MAP_IS_EMPTY(m) (map_is_empty((map_t)m))

#define MAP_SIZE(m) (map_size((map_t)m))

// THROWS: [MALLOC_EXCEPTION]
#define MAP_PUT(m, k, v) (m->put(m, k, v))

#define MAP_GET(m, k) (m->get(m, k))

#define MAP_EXISTS(m, k) (m->exists(m, k))

#define MAP_REMOVE(m, k) (m->remove(m, k))

#endif //LLP_LAB1_MAP_H
