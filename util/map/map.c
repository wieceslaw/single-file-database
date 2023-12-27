//
// Created by vyach on 04.10.2023.
//

#include <stdlib.h>
#include <assert.h>
#include "map.h"
#include "../exceptions/exceptions.h"

struct map_iterator {
    map_t map;
    size_t bucket_idx;
    ListIterator bucket_it;
};

static void map_entry_free(map_t map, map_entry_t *entry) {
    assert(entry != NULL);
    map->key_free((*entry)->key);
    map->val_free((*entry)->val);
    free(*entry);
    *entry = NULL;
}

// THROWS: [MALLOC_EXCEPTION]
static map_entry_t map_entry_init(map_t map, map_key key, map_value value) {
    assert(map != NULL);
    map_entry_t entry = rmalloc(sizeof(struct map_entry));
    entry->key = map->key_copy(key);
    entry->val = map->val_copy(value);
    return entry;
}

// THROWS: [MALLOC_EXCEPTION]
static List *buckets_init(size_t capacity) {
    List *buckets = rmalloc(sizeof(List) * capacity);
    for (size_t i = 0; i < capacity; i++) {
        buckets[i] = ListNew();
    }
    return buckets;
}

static void buckets_free(List *buckets, size_t capacity) {
    if (NULL == buckets) {
        return;
    }
    for (size_t i = 0; i < capacity; i++) {
        ListFree(buckets[i]);
        buckets[i] = NULL;
    }
    free(buckets);
}

// THROWS: [MALLOC_EXCEPTION]
map_t map_init(size_t capacity, hash_f key_hash, equals_f key_equals,
               copy_f key_copy, free_f key_free, copy_f val_copy, free_f val_free) {
    capacity = MAX(capacity, MAP_START_CAPACITY);
    List *buckets;
    map_t map;
    TRY({
        buckets = buckets_init(capacity);
        map = rmalloc(sizeof(*map));
    }) CATCH(exception == MALLOC_EXCEPTION, {
        buckets_free(buckets, capacity);
        free(map);
        RAISE(exception);
    }) FINALLY()
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
    assert(map != NULL);
    for (size_t i = 0; i < map->capacity; i++) {
        List list = map->buckets[i];
        FOR_LIST(list, it, {
            map_entry_t entry = ListIteratorGet(it);
            map->val_free(entry->val);
            map->key_free(entry->key);
            free(entry);
        })
        ListFree(list);
        list = NULL;
    }
    free(map->buckets);
    map->buckets = NULL;
}

void map_free(map_t *map) {
    assert(map != NULL);
    if (*map == NULL) {
        return;
    }
    array_free(*map);
    (*map)->buckets = NULL;
    (*map)->size = 0;
    (*map)->capacity = 0;
    free(*map);
    *map = NULL;
}

size_t map_size(map_t map) {
    assert(map != NULL);
    return map->size;
}

bool map_is_empty(map_t map) {
    assert(map != NULL);
    return 0 == map_size(map);
}

bool map_exists(map_t map, map_key key) {
    assert(map != NULL && key != NULL);
    size_t hash = map->key_hash(key);
    size_t idx = hash % map->capacity;
    List list = map->buckets[idx];
    bool exists = false;
    FOR_LIST(list, it, {
        map_entry_t entry = ListIteratorGet(it);
        if (map->key_equals(entry->key, key)) {
            exists = true;
            break;
        }
    })
    return exists;
}

map_value map_get(map_t map, map_key key) {
    assert(map != NULL && key != NULL);
    size_t hash = map->key_hash(key);
    size_t idx = hash % map->capacity;
    List list = map->buckets[idx];
    map_value value = NULL;
    FOR_LIST(list, it, {
        map_entry_t entry = ListIteratorGet(it);
        if (map->key_equals(entry->key, key)) {
            value = map->val_copy(entry->val);
        }
    })
    return value;
}

void map_remove(map_t map, map_key key) {
    assert(map != NULL && key != NULL);
    size_t hash = map->key_hash(key);
    size_t idx = hash % map->capacity;
    List list = map->buckets[idx];
    FOR_LIST(list, it, {
        map_entry_t entry = ListIteratorGet(it);
        if (map->key_equals(entry->key, key)) {
            map_entry_free(map, &entry);
            ListIteratorDeleteNode(it);
            map->size--;
            break;
        }
    })
}

static bool map_should_resize(map_t map) {
    assert(map != NULL);
    return (double) map->size >= (double) map->capacity * MAP_LOAD_FACTOR;
}

// THROWS: [MALLOC_EXCEPTION]
static void map_resize(map_t map) {
    assert(map != NULL);
    size_t capacity = (size_t) map->capacity * MAP_EXTENSION_RATIO;
    List *buckets;
    TRY ({
        buckets = buckets_init(capacity);
        FOR_MAP(map, e, {
            size_t hash = map->key_hash(e->key);
            size_t idx = hash % capacity;
            List list = buckets[idx];
            ListAppendTail(list, e);
        })
        array_free(map);
        map->capacity = capacity;
        map->buckets = buckets;
    }) CATCH(exception == MALLOC_EXCEPTION, {
        buckets_free(buckets, capacity);
        RAISE(exception);
    }) FINALLY ()
}

// THROWS: [MALLOC_EXCEPTION]
void map_put(map_t map, map_key key, map_value value) {
    assert(map != NULL && key != NULL);
    if (map_should_resize(map)) {
        map_resize(map);
    }
    size_t hash = map->key_hash(key);
    size_t idx = hash % map->capacity;
    List list = map->buckets[idx];
    bool added = false;
    FOR_LIST(list, it, {
        map_entry_t entry = ListIteratorGet(it);
        if (map->key_equals(entry->key, key)) {
            map->val_free(entry->val);
            entry->val = map->val_copy(value);
            added = true;
        }
    })
    if (added) {
        return;
    }
    map_entry_t entry = map_entry_init(map, key, value);
    ListAppendTail(list, entry);
    map->size++;
}

// THROWS: [MALLOC_EXCEPTION]
map_it map_get_iterator(map_t map) {
    assert(map != NULL);
    map_it it;
    it = rmalloc(sizeof(struct map_iterator));
    it->map = map;
    it->bucket_idx = 0;
    it->bucket_it = ListHeadIterator(map->buckets[it->bucket_idx]);
    if (ListIteratorIsEmpty(it->bucket_it)) {
        map_it_next(it);
    }
    return it;
}

void map_it_free(map_it *it) {
    assert(NULL != it);
    if (NULL == *it) {
        return;
    }
    ListIteratorFree(&(*it)->bucket_it);
    free(*it);
    *it = NULL;
}

bool map_it_is_empty(map_it it) {
    assert(it != NULL);
    return it->bucket_idx == it->map->capacity;
}

void map_it_next(map_it it) {
    assert(it != NULL);
    if (it->bucket_it != NULL) {
        ListIteratorNext(it->bucket_it);
    }
    while (it->bucket_it != NULL && ListIteratorIsEmpty(it->bucket_it)) {
        ListIteratorFree(&(it->bucket_it));
        it->bucket_idx++;
        if (it->bucket_idx < it->map->capacity) {
            it->bucket_it = ListHeadIterator(it->map->buckets[it->bucket_idx]);
        }
    }
}

// THROWS: [MALLOC_EXCEPTION]
map_entry_t map_it_get_entry(map_it it) {
    assert(it != NULL);
    if (map_it_is_empty(it)) {
        return NULL;
    }
    map_entry_t entry = ListIteratorGet(it->bucket_it);
    map_entry_t entry_copy = map_entry_init(it->map, entry->key, entry->val);
    return entry_copy;
}
