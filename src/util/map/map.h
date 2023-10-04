//
// Created by vyach on 04.10.2023.
//

#ifndef LLP_LAB1_MAP_H
#define LLP_LAB1_MAP_H

#include <stddef.h>
#include <stdbool.h>
#include <stdint-gcc.h>

#define MAP_START_CAPACITY 8
#define MAP_EXTENSION_RATIO 2

typedef size_t (*hash_f)(void *);

typedef bool (*equals_f)(void *, void *);

typedef void (*free_f)(void *);

typedef void *(*copy_f)(void *);

typedef void *map_key;

typedef void *map_value;

#define MAP_PUT_SIGNATURE(M, K, V, name) bool(*name)(M, K, V)

#define MAP_GET_SIGNATURE(M, K, V, name) V(*name)(M, K)

#define MAP_EXISTS_SIGNATURE(M, K, V, name) bool(*name)(M, K)

#define MAP_REMOVE_SIGNATURE(M, K, V, name) void(*name)(M, K)

typedef struct generic_map {
    MAP_PUT_SIGNATURE(struct generic_map*, map_key, map_value, put);

    MAP_GET_SIGNATURE(struct generic_map*, map_key, map_value, get);

    MAP_EXISTS_SIGNATURE(struct generic_map*, map_key, map_value, exists);

    MAP_REMOVE_SIGNATURE(struct generic_map*, map_key, map_value, remove);

    map_key *array;
    size_t capacity;
    size_t size;
    hash_f key_hash;
    equals_f key_equals;
    copy_f key_copy;
    free_f key_free;
    copy_f val_copy;
    free_f val_free;

} *generic_map_t;

generic_map_t generic_map_init(size_t capacity, hash_f key_hash, equals_f key_equals,
                               copy_f key_copy, free_f key_free, copy_f val_copy, free_f val_free);

void generic_map_free(generic_map_t map);

size_t generic_map_size(generic_map_t map);

bool generic_map_is_empty(generic_map_t map);

void generic_map_remove(generic_map_t map, map_key key);

bool generic_map_put(generic_map_t map, map_key key, map_value value);

map_value generic_map_get(generic_map_t map, map_key key);

bool generic_map_exists(generic_map_t map, map_key key);

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
    ((T)generic_map_init(CAPACITY, HASH_F, EQUALS_F, KEY_COPY, KEY_FREE, VAL_COPY, VAL_FREE)) \

#define MAP_FREE(m) (generic_map_free(m))

#define MAP_IS_EMPTY(m) (generic_map_is_empty(m))

#define MAP_SIZE(m) (generic_map_size(m))

#define MAP_PUT(m, k, v) (m->put(m, k, v))

#define MAP_GET(m, k) (m->get(m, k))

#define MAP_EXISTS(m, k) (m->exists(m, k))

#define MAP_REMOVE(m, k) (m->remove(m, k))

#endif //LLP_LAB1_MAP_H
