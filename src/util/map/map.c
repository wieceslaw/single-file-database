//
// Created by vyach on 04.10.2023.
//

#include <stdlib.h>
#include <string.h>
#include "map.h"
#include "util/defines.h"

generic_map_t generic_map_init(size_t capacity, hash_f key_hash, equals_f key_equals,
                               copy_f key_copy, free_f key_free, copy_f val_copy, free_f val_free) {

    capacity = MAX(capacity, MAP_START_CAPACITY);
    map_key *array = malloc(sizeof(map_key) * capacity);
    if (NULL == array) {
        return NULL;
    }
    memset(array, 0, sizeof(map_key) * capacity);
    generic_map_t map = malloc(sizeof(*map));
    if (NULL == map) {
        free(array);
        return NULL;
    }
    map->array = array;
    map->capacity = capacity;
    map->size = 0;
    map->key_hash = key_hash;
    map->key_equals = key_equals;
    map->key_copy = key_copy;
    map->key_free = key_free;
    map->val_copy = val_copy;
    map->val_free = val_free;
    map->get = &generic_map_get;
    map->put = &generic_map_put;
    map->remove = &generic_map_remove;
    map->exists = &generic_map_exists;
    return map;
}

static void array_free(generic_map_t map) {
    // TODO: Implement
}

void generic_map_free(generic_map_t map) {
    array_free(map);
    map->array = NULL;
    map->size = 0;
    map->capacity = 0;
    free(map);
}

size_t generic_map_size(generic_map_t map) {
    return map->size;
}

bool generic_map_is_empty(generic_map_t map) {
    return 0 == generic_map_size(map);
}

// TODO: Implement
void generic_map_remove(generic_map_t map, map_key key);

bool generic_map_put(generic_map_t map, map_key key, map_value value);

map_value generic_map_get(generic_map_t map, map_key key);

bool generic_map_exists(generic_map_t map, map_key key);
