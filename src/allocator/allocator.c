//
// Created by vyach on 15.09.2023.
//

#include <stdbool.h>
#include "allocator/allocator.h"
#include "file/file.h"

typedef struct free_block {
    offset_t next;
} free_block_h;

#ifdef _WIN32

#include "windows.h"

typedef struct block_list_node block_list_node;

struct block_list_node {
    block *block;
    block_list_node *next;
    block_list_node *prev;
};

typedef struct {
    block_list_node *head;
    block_list_node *tail;
} block_list;

typedef struct {
    block_list *list;
    block_list_node *node;
} block_list_it;

struct block {
    void *ptr;
    offset_t offset;
    block_list_node *node;
};

bool block_list_append(block_list *list, block *block) {
    block_list_node *node = malloc(sizeof(block_list_node));
    if (node == NULL) {
        return false;
    }
    block->node = node;
    if (list->head == NULL) {
        list->head = node;
    } else {
        block_list_node *last_node = list->tail;
        last_node->next = node;
        node->prev = last_node;
    }
    list->tail = node;
}

void block_list_delete(block_list *list, block_list_node *node) {
    if (node == NULL) return;
    block_list_node *prev = node->prev;
    block_list_node *next = node->next;
    if (prev != NULL && next != NULL) {
        prev->next = next;
        next->prev = prev;
    }
    if (prev == NULL) {
        list->head = next;
        if (next != NULL) {
            next->prev = NULL;
        }
    }
    if (next == NULL) {
        list->tail = prev;
        if (prev != NULL) {
            prev->next = NULL;
        }
    }
    node->block->node = NULL;
    free(node);
}

void block_list_iterator(block_list *list, block_list_it *it) {
    it->list = list;
    it->node = list->head;
}

bool block_list_iterator_is_empty(block_list_it * it) {
    return it->node == NULL;
}

void block_list_iterator_next(block_list_it * it) {
    if (block_list_iterator_is_empty(it)) return;
    it->node = it->node->next;
}

block *block_list_iterator_get(block_list_it * it) {
    if (block_list_iterator_is_empty(it)) {
        return NULL;
    }
    return it->node->block;
}

offset_t block_offset(block *block) {
    return block->offset;
}

void *block_ptr(block *block) {
    return block->ptr;
}

struct allocator {
    HANDLE hFile;
    HANDLE hMap;
    LARGE_INTEGER liFileSize;
    block_list block_list;
    void *mapped_header;
};

static offset_t *p_allocator_free_blocks_next(allocator *allocator) {
    return allocator->mapped_header + offsetof(file_header, free_blocks_next);
}

static uint32_t *p_allocator_free_blocks_count(allocator *allocator) {
    return allocator->mapped_header + offsetof(file_header, free_blocks_count);
}

static void *p_map_block(allocator *allocator, offset_t offset) {
    ULARGE_INTEGER ulOffset;
    ulOffset.QuadPart = offset;
    return MapViewOfFile(
            allocator->hMap,
            FILE_MAP_READ | FILE_MAP_WRITE,
            ulOffset.HighPart,
            ulOffset.LowPart,
            BLOCK_SIZE);
}

block *allocator_map_block(allocator *allocator, offset_t offset) {
    void *ptr = p_map_block(allocator, offset);
    if (ptr == NULL) {
        return NULL;
    }
    block *block_ptr = malloc(sizeof(block));
    if (block_ptr == NULL) {
        UnmapViewOfFile(ptr);
        return NULL;
    }
    block_ptr->offset = offset;
    block_ptr->ptr = ptr;
    block_list_append(&allocator->block_list, block_ptr);
    return block_ptr;
}

allocator_result allocator_unmap_block(allocator *allocator, block *block) {
    if (block == NULL) return ALLOCATOR_SUCCESS;
    block_list_delete(&allocator->block_list, block->node);
    if (UnmapViewOfFile(block->ptr) != 0) {
        free(block);
        return ALLOCATOR_UNABLE_UNMAP;
    }
    free(block);
    return ALLOCATOR_SUCCESS;
}

allocator_result allocator_collect_block(allocator *allocator, offset_t offset) {
    block *block = allocator_map_block(allocator, offset);
    if (block == NULL) {
        return ALLOCATOR_UNABLE_MAP;
    }
    free_block_h *free_block = block->ptr;
    free_block->next = *p_allocator_free_blocks_next(allocator);
    *p_allocator_free_blocks_next(allocator) = offset;
    if (allocator_unmap_block(allocator, block) != ALLOCATOR_SUCCESS) {
        return ALLOCATOR_UNABLE_UNMAP;
    }
    *p_allocator_free_blocks_count(allocator) += 1;
    return ALLOCATOR_SUCCESS;
}

static allocator_result p_clear_mapping(allocator *allocator) {
    block_list_it it;
    block_list_iterator(&allocator->block_list, &it);
    while (block_list_iterator_is_empty(&it)) {
        block *block = block_list_iterator_get(&it);
        if (UnmapViewOfFile(block->ptr) != 0) {
            return ALLOCATOR_UNABLE_EXTEND;
        }
        block->ptr = NULL;
        block_list_iterator_next(&it);
    }
    if (UnmapViewOfFile(allocator->mapped_header) != 0) {
        return ALLOCATOR_UNABLE_EXTEND;
    }
    allocator->mapped_header = NULL;
    CloseHandle(allocator->hMap);
    return ALLOCATOR_SUCCESS;
}

static allocator_result p_fill_mapping(allocator *allocator) {
    allocator->hMap = CreateFileMapping(
            allocator->hFile,
            NULL,
            PAGE_READWRITE,
            0,
            0,
            NULL);
    if (allocator->hMap == 0) {
        return ALLOCATOR_UNABLE_MAP;
    }
    block_list_it it;
    block_list_iterator(&allocator->block_list, &it);
    while (block_list_iterator_is_empty(&it)) {
        block *block = block_list_iterator_get(&it);
        void *ptr = p_map_block(allocator, block->offset);
        if (ptr == NULL) {
            return ALLOCATOR_UNABLE_MAP;
        }
        block->ptr = ptr;
        block_list_iterator_next(&it);
    }
    allocator->mapped_header = p_map_block(allocator, 0);
    return ALLOCATOR_SUCCESS;
}

allocator_result allocator_reserve_blocks(allocator *allocator, uint32_t n) {
    uint32_t count = *p_allocator_free_blocks_count(allocator);
    if (count >= n) {
        return ALLOCATOR_SUCCESS;
    }
    uint32_t added = n - count;
    offset_t old_size = allocator->liFileSize.QuadPart;
    offset_t new_size = old_size + (BLOCK_SIZE * added);
    if (p_clear_mapping(allocator) != ALLOCATOR_SUCCESS) {
        return ALLOCATOR_UNABLE_UNMAP;
    }
    struct {
        struct {
            LONG LowPart;
            LONG HighPart;
        } u;
        LONGLONG QuadPart;
    } liOffset;
    liOffset.QuadPart = new_size;
    if (SetFilePointer(
            allocator->hFile,
            liOffset.u.LowPart,
            &liOffset.u.HighPart,
            FILE_BEGIN
    ) == INVALID_SET_FILE_POINTER) {
        return ALLOCATOR_UNABLE_EXTEND;
    }
    if (SetEndOfFile(allocator->hFile)) {
        return ALLOCATOR_UNABLE_EXTEND;
    }
    if (p_fill_mapping(allocator) != ALLOCATOR_SUCCESS) {
        return ALLOCATOR_UNABLE_MAP;
    }
    allocator->liFileSize.QuadPart = new_size;
    *p_allocator_free_blocks_count(allocator) += added;
    for (offset_t offset = old_size; offset < new_size; offset += BLOCK_SIZE) {
        if (allocator_collect_block(allocator, offset) != ALLOCATOR_SUCCESS) {
            return ALLOCATOR_UNABLE_EXTEND;
        }
    }
    return ALLOCATOR_SUCCESS;
}

block *allocator_get_block(allocator *allocator) {
    if (*p_allocator_free_blocks_count(allocator) == 0) {
        if (allocator_reserve_blocks(allocator, 1) != ALLOCATOR_SUCCESS) {
            return NULL;
        }
    }
    offset_t next_block_offset = *p_allocator_free_blocks_next(allocator);
    block *free_block = allocator_map_block(allocator, next_block_offset);
    if (free_block == NULL) {
        return NULL;
    }
    *p_allocator_free_blocks_count(allocator) -= 1;
    *p_allocator_free_blocks_next(allocator) = ((free_block_h *) free_block->ptr)->next;
    return free_block;
}

#else

#include <sys/mman.h>
#include <stddef.h>
#include <sys/stat.h>
#include <unistd.h>
#include <malloc.h>

struct allocator {
    int fd;
    off_t file_size;
    offset_t *free_blocks_next;
    uint32_t *free_blocks_count;
};

struct block {
    void *ptr;
    offset_t offset;
};

offset_t block_offset(block * block) {
    return block->offset;
}

void *block_ptr(block * block) {
    return block->ptr;
}

block *allocator_map_block(allocator *allocator, offset_t offset) {
    void *block_mapping = mmap(NULL, BLOCK_SIZE, PROT_READ | PROT_WRITE, MAP_SHARED, allocator->fd, offset);
    if (block_mapping == MAP_FAILED) {
        return NULL;
    }
    block *block_ptr = malloc(sizeof(block));
    if (block_ptr == NULL) {
        munmap(block_mapping, BLOCK_SIZE);
        return NULL;
    }
    block_ptr->ptr = block_mapping;
    block_ptr->offset = offset;
    return block_ptr;
}

allocator_result allocator_unmap_block(allocator *allocator, block *block) {
    if (munmap(block->ptr, BLOCK_SIZE) != 0) {
        free(block);
        return ALLOCATOR_UNABLE_UNMAP;
    }
    free(block);
    return ALLOCATOR_SUCCESS;
}

allocator_result allocator_collect_block(allocator *allocator, offset_t offset) {
    block *block = allocator_map_block(allocator, offset);
    if (block == NULL) {
        return ALLOCATOR_UNABLE_MAP;
    }
    free_block_h *free_block = block->ptr;
    free_block->next = *allocator->free_blocks_next;
    *allocator->free_blocks_next = offset;
    if (allocator_unmap_block(allocator, block) != ALLOCATOR_SUCCESS) {
        return ALLOCATOR_UNABLE_UNMAP;
    }
    *allocator->free_blocks_count += 1;
    return ALLOCATOR_SUCCESS;
}

allocator_result allocator_reserve_blocks(allocator *allocator, uint32_t n) {
    if (*allocator->free_blocks_count >= n) {
        return ALLOCATOR_SUCCESS;
    }
    uint32_t added = n - *allocator->free_blocks_count;
    offset_t old_size = allocator->file_size;
    offset_t new_size = old_size + (BLOCK_SIZE * added);
    if (ftruncate(allocator->fd, new_size) != 0) {
        return ALLOCATOR_UNABLE_EXTEND;
    }
    allocator->file_size = new_size;
    *allocator->free_blocks_count += added;
    for (offset_t offset = old_size; offset < new_size; offset += BLOCK_SIZE) {
        if (allocator_collect_block(allocator, offset) != ALLOCATOR_SUCCESS) {
            return ALLOCATOR_UNABLE_EXTEND;
        }
    }
    return ALLOCATOR_SUCCESS;
}

block *allocator_get_block(allocator *allocator) {
    if (*allocator->free_blocks_count == 0) {
        if (allocator_reserve_blocks(allocator, 1) != ALLOCATOR_SUCCESS) {
            return NULL;
        }
    }
    offset_t next_block_offset = *allocator->free_blocks_next;
    block *free_block = allocator_map_block(allocator, next_block_offset);
    if (free_block == NULL) {
        return NULL;
    }
    *allocator->free_blocks_count -= 1;
    *allocator->free_blocks_next = ((free_block_h *) free_block->ptr)->next;
    return free_block;
}

#endif
