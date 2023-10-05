//
// Created by vyach on 04.10.2023.
//

#include <stdlib.h>
#include <assert.h>
#include "map.h"
#include "util/defines.h"
#include "util/exceptions/exceptions.h"

struct map_iterator {
    map_t map;
    size_t bucket_idx;
    list_it bucket_it;
};

static void map_entry_free(map_t map, map_entry_t *entry) {
    map->key_free((*entry)->key);
    map->val_free((*entry)->val);
    free(entry);
    entry = NULL;
}

// THROWS: [MALLOC_EXCEPTION]
static map_entry_t map_entry_init(map_t map, map_key key, map_value value) {
    map_entry_t entry = rmalloc(sizeof(struct map_entry));
    entry->key = map->key_copy(key);
    entry->val = map->val_copy(value);
    return entry;
}

// THROWS: [MALLOC_EXCEPTION]
static list_t *buckets_init(size_t capacity) {
    list_t *buckets = rmalloc(sizeof(list_t) * capacity);
    for (size_t i = 0; i < capacity; i++) {
        buckets[i] = list_init();
    }
    return buckets;
}

static void buckets_free(list_t *buckets, size_t capacity) {
    for (size_t i = 0; i < capacity; i++) {
        list_free(&buckets[i]);
    }
    free(buckets);
}

// THROWS: [MALLOC_EXCEPTION]
map_t map_init(size_t capacity, hash_f key_hash, equals_f key_equals,
               copy_f key_copy, free_f key_free, copy_f val_copy, free_f val_free) {
    capacity = MAX(capacity, MAP_START_CAPACITY);
    list_t *buckets;
    map_t map;
    TRY({
        buckets = buckets_init(capacity);
        map = rmalloc(sizeof(*map));
    }) FINALLY({
        buckets_free(buckets, capacity);
        free(map);
    })
    map->buckets = buckets;
    map->capacity = capacity;
    map->size = 0;
    map->key_hash = key_hash;
    map->key_equals = key_equals;
    map->key_copy = key_copy;
    map->key_free = key_free;
    map->val_copy = val_copy;
    map->val_free = val_free;
    map->get = &map_get;
    map->put = &map_put;
    map->remove = &map_remove;
    map->exists = &map_exists;
    return map;
}

static void array_free(map_t map) {
    for (size_t i = 0; i < map->capacity; i++) {
        list_free(&map->buckets[i]);
    }
}

void map_free(map_t *map) {
    if (NULL == map) {
        return;
    }
    array_free(*map);
    (*map)->buckets = NULL;
    (*map)->size = 0;
    (*map)->capacity = 0;
    free(*map);
    map = NULL;
}

size_t generic_map_size(map_t map) {
    assert(map != NULL);
    return map->size;
}

bool map_is_empty(map_t map) {
    assert(map != NULL);
    return 0 == generic_map_size(map);
}

bool map_exists(map_t map, map_key key) {
    assert(map != NULL && key != NULL);
    size_t hash = map->key_hash(key);
    size_t idx = hash % map->capacity;
    list_t list = map->buckets[idx];
    FOR_LIST(list, it, {
        map_entry_t entry = list_it_get(it);
        if (entry->key == key) {
            return true;
        }
    })
    return false;
}

map_value map_get(map_t map, map_key key) {
    assert(map != NULL && key != NULL);
    size_t hash = map->key_hash(key);
    size_t idx = hash % map->capacity;
    list_t list = map->buckets[idx];
    FOR_LIST(list, it, {
        map_entry_t entry = list_it_get(it);
        if (entry->key == key) {
            return entry->val;
        }
    })
    return NULL;
}

void map_remove(map_t map, map_key key) {
    assert(map != NULL && key != NULL);
    size_t hash = map->key_hash(key);
    size_t idx = hash % map->capacity;
    list_t list = map->buckets[idx];
    FOR_LIST(list, it, {
        map_entry_t entry = list_it_get(it);
        if (entry->key == key) {
            map_entry_free(map, &entry);
            list_it_delete(it);
            map->size--;
            return;
        }
    })
}

static bool map_should_resize(map_t map) {
    return (double) map->size >= (double) map->capacity * MAP_LOAD_FACTOR;
}

// THROWS: [MALLOC_EXCEPTION]
static void map_resize(map_t map) {
    size_t capacity = map->capacity * MAP_EXTENSION_RATIO;
    list_t *buckets;
    TRY ({
        buckets = buckets_init(capacity);
        FOR_MAP(map, e, {
            size_t hash = map->key_hash(e->key);
            size_t idx = hash % capacity;
            list_t list = buckets[idx];
            list_append_tail(list, e);
        })
    }) FINALLY ({
        buckets_free(buckets, capacity);
    })
}

// THROWS: [MALLOC_EXCEPTION]
void map_put(map_t map, map_key key, map_value value) {
    assert(map != NULL && key != NULL);
    if (map_should_resize(map)) {
        map_resize(map);
    }
    size_t hash = map->key_hash(key);
    size_t idx = hash % map->capacity;
    list_t list = map->buckets[idx];
    FOR_LIST(list, it, {
        map_entry_t entry = list_it_get(it);
        if (entry->key == key) {
            map->val_free(entry->val);
            entry->val = map->val_copy(value);
            return;
        }
    })
    map_entry_t entry = map_entry_init(map, key, value);
    list_append_tail(list, entry);
    map->size++;
}

// THROWS: [MALLOC_EXCEPTION]
map_it map_get_iterator(map_t map) {
    map_it it;
    it = rmalloc(sizeof(struct map_iterator));
    it->map = map;
    it->bucket_idx = 0;
    it->bucket_it = list_head_iterator(map->buckets[it->bucket_idx]);
    return it;
}

void map_it_free(map_it *it) {
    free(*it);
    it = NULL;
}

bool map_it_is_empty(map_it it) {
    return it->bucket_idx == it->map->capacity;
}

void map_it_next(map_it it) {
    list_it_next(it->bucket_it);
    if (list_it_is_empty(it->bucket_it)) {
        list_it_free(&(it->bucket_it));
        it->bucket_idx++;
        if (it->bucket_idx < it->map->capacity) {
            it->bucket_it = list_head_iterator(it->map->buckets[it->bucket_idx]);
        }
    }
}

// THROWS: [MALLOC_EXCEPTION]
map_entry_t map_it_get_entry(map_it it) {
    map_entry_t entry = list_it_get(it->bucket_it);
    map_entry_t entry_copy = map_entry_init(it->map, entry->key, entry->val);
    return entry_copy;
}
