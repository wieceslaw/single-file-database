//
// Created by vyach on 15.09.2023.
//

#include <stdbool.h>
#include <stdio.h>
#include "allocator/allocator.h"

typedef struct free_block {
    offset_t next;
} free_block_h;

void *block_addr_get(block_addr *block_addr) {
    return block_ptr(block_addr->block) + block_addr->offset;
}

static bool header_is_valid(file_h *header) {
    return header->magic == MAGIC;
}

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
    *node = (block_list_node) {0};
    if (node == NULL) {
        return false;
    }
    block->node = node;
    node->block = block;
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

bool block_list_iterator_is_empty(block_list_it *it) {
    return it->node == NULL;
}

void block_list_iterator_next(block_list_it *it) {
    if (block_list_iterator_is_empty(it)) {
        return;
    }
    it->node = it->node->next;
}

block *block_list_iterator_get(block_list_it *it) {
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
    file_h *header;
};

static void *map_block(allocator *allocator, offset_t offset) {
    ULARGE_INTEGER ulOffset;
    ulOffset.QuadPart = offset;
    return MapViewOfFile(
            allocator->hMap,
            FILE_MAP_READ | FILE_MAP_WRITE,
            ulOffset.HighPart,
            ulOffset.LowPart,
            BLOCK_SIZE);
}

file_status allocator_init(file_settings *settings, allocator **allocator_ptr) {
    HANDLE hFile;
    LARGE_INTEGER fileSize;
    HANDLE hMap;
    file_h *header;
    switch (settings->open_type) {
        case FILE_OPEN_EXIST: {
            hFile = CreateFile(settings->path,
                               GENERIC_READ | GENERIC_WRITE,
                               FILE_SHARE_READ | FILE_SHARE_WRITE,
                               NULL,
                               OPEN_EXISTING,
                               FILE_ATTRIBUTE_NORMAL,
                               0);
            if (INVALID_HANDLE_VALUE == hFile) {
                return FILE_ST_ALREADY_EXISTS;
            }
            break;
        }
        case FILE_OPEN_CLEAR: {
            hFile = CreateFile(settings->path,
                               GENERIC_READ | GENERIC_WRITE,
                               FILE_SHARE_READ | FILE_SHARE_WRITE,
                               NULL,
                               OPEN_ALWAYS,
                               FILE_ATTRIBUTE_NORMAL,
                               0);
            if (INVALID_HANDLE_VALUE == hFile) {
                return FILE_ST_UNABLE_OPEN;
            }
            break;
        }
        case FILE_OPEN_CREATE: {
            hFile = CreateFile(settings->path,
                               GENERIC_READ | GENERIC_WRITE,
                               FILE_SHARE_READ | FILE_SHARE_WRITE,
                               NULL,
                               CREATE_NEW,
                               FILE_ATTRIBUTE_NORMAL,
                               0);
            if (INVALID_HANDLE_VALUE == hFile) {
                return FILE_ST_ALREADY_EXISTS;
            }
            break;
        }
    }
    if (!GetFileSizeEx(hFile, &fileSize)) {
        CloseHandle(hFile);
        return FILE_ST_ERROR;
    }
    switch (settings->open_type) {
        case FILE_OPEN_EXIST: {
            if (fileSize.QuadPart < BLOCK_SIZE) {
                CloseHandle(hFile);
                return FILE_ST_WRONG_FORMAT;
            }
            hMap = CreateFileMapping(
                    hFile,
                    NULL,
                    PAGE_READWRITE,
                    0,
                    0,
                    NULL);
            if (0 == hMap) {
                CloseHandle(hFile);
                return FILE_ST_ERROR;
            }
            header = MapViewOfFile(
                    hMap,
                    FILE_MAP_READ | FILE_MAP_WRITE,
                    0,
                    0,
                    BLOCK_SIZE);
            if (NULL == header) {
                CloseHandle(hMap);
                CloseHandle(hFile);
                return FILE_ST_ERROR;
            }
            if (!header_is_valid(header)) {
                CloseHandle(hMap);
                CloseHandle(hFile);
                return FILE_ST_WRONG_FORMAT;
            }
            break;
        }
        case FILE_OPEN_CREATE:
        case FILE_OPEN_CLEAR: {
            if (INVALID_SET_FILE_POINTER ==
                SetFilePointer(hFile, BLOCK_SIZE, NULL, FILE_BEGIN)) {
                return FILE_ST_ERROR;
            }
            if (!SetEndOfFile(hFile)) {
                return FILE_ST_ERROR;
            }
            hMap = CreateFileMapping(
                    hFile,
                    NULL,
                    PAGE_READWRITE,
                    0,
                    0,
                    NULL);
            if (0 == hMap) {
                CloseHandle(hFile);
                return FILE_ST_ERROR;
            }
            header = MapViewOfFile(
                    hMap,
                    FILE_MAP_READ | FILE_MAP_WRITE,
                    0,
                    0,
                    BLOCK_SIZE);
            if (NULL == header) {
                CloseHandle(hMap);
                CloseHandle(hFile);
                return FILE_ST_ERROR;
            }
            *header = (file_h) {0};
            header->magic = MAGIC;
            fileSize.QuadPart = BLOCK_SIZE;
            break;
        }
    }
    *allocator_ptr = malloc(BLOCK_SIZE);
    if (allocator_ptr == NULL) {
        return FILE_ST_ERROR;
    }
    **allocator_ptr = (allocator) {
            .header = header,
            .hFile = hFile,
            .hMap = hMap,
            .liFileSize = fileSize,
            .block_list = (block_list) {0}
    };
    return FILE_ST_OK;
}

file_status allocator_free(allocator *allocator) {
    if (!UnmapViewOfFile(allocator->header)) {
        CloseHandle(allocator->hMap);
        CloseHandle(allocator->hFile);
        free(allocator);
        return FILE_ST_UNABLE_RELEASE;
    }
    if (!CloseHandle(allocator->hMap)) {
        CloseHandle(allocator->hFile);
        free(allocator);
        return FILE_ST_UNABLE_RELEASE;
    }
    if (!CloseHandle(allocator->hFile)) {
        free(allocator);
        return FILE_ST_UNABLE_RELEASE;
    }
    free(allocator);
    return FILE_ST_OK;
}

block *allocator_map_block(allocator *allocator, offset_t offset) {
    void *ptr = map_block(allocator, offset);
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
    if (!UnmapViewOfFile(block->ptr)) {
        free(block);
        return ALLOCATOR_UNABLE_UNMAP;
    }
    free(block);
    return ALLOCATOR_SUCCESS;
}

allocator_result allocator_return_block(allocator *allocator, offset_t offset) {
    block *block = allocator_map_block(allocator, offset);
    if (block == NULL) {
        return ALLOCATOR_UNABLE_MAP;
    }
    free_block_h *free_block = block->ptr;
    free_block->next = allocator->header->free_blocks_next;
    allocator->header->free_blocks_next = offset;
    if (allocator_unmap_block(allocator, block) != ALLOCATOR_SUCCESS) {
        return ALLOCATOR_UNABLE_UNMAP;
    }
    allocator->header->free_blocks_count += 1;
    return ALLOCATOR_SUCCESS;
}

static allocator_result clear_mapping(allocator *allocator) {
    block_list_it it;
    block_list_iterator(&allocator->block_list, &it);
    while (!block_list_iterator_is_empty(&it)) {
        block *block = block_list_iterator_get(&it);
        if (!UnmapViewOfFile(block->ptr)) {
            return ALLOCATOR_UNABLE_EXTEND;
        }
        block->ptr = NULL;
        block_list_iterator_next(&it);
    }
    if (!UnmapViewOfFile(allocator->header)) {
        return ALLOCATOR_UNABLE_EXTEND;
    }
    allocator->header = NULL;
    if (!CloseHandle(allocator->hMap)) {
        return ALLOCATOR_UNABLE_EXTEND;
    }
    return ALLOCATOR_SUCCESS;
}

static allocator_result fill_mapping(allocator *allocator) {
    allocator->hMap = CreateFileMapping(
            allocator->hFile,
            NULL,
            PAGE_READWRITE,
            0,
            0,
            NULL);
    if (allocator->hMap == INVALID_HANDLE_VALUE) {
        return ALLOCATOR_UNABLE_MAP;
    }
    block_list_it it;
    block_list_iterator(&allocator->block_list, &it);
    while (!block_list_iterator_is_empty(&it)) {
        block *block = block_list_iterator_get(&it);
        void *ptr = map_block(allocator, block->offset);
        if (ptr == NULL) {
            return ALLOCATOR_UNABLE_MAP;
        }
        block->ptr = ptr;
        block_list_iterator_next(&it);
    }
    allocator->header = map_block(allocator, 0);
    if (allocator->header == NULL) {
        return ALLOCATOR_UNABLE_MAP;
    }
    return ALLOCATOR_SUCCESS;
}

allocator_result allocator_reserve_blocks(allocator *allocator, uint32_t n) {
    uint32_t count = allocator->header->free_blocks_count;
    if (count >= n) {
        return ALLOCATOR_SUCCESS;
    }
    uint32_t added = n - count;
    offset_t old_size = allocator->liFileSize.QuadPart;
    offset_t new_size = old_size + (BLOCK_SIZE * added);
    if (clear_mapping(allocator) != ALLOCATOR_SUCCESS) {
        return ALLOCATOR_UNABLE_UNMAP;
    }
    union {
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
    if (!SetEndOfFile(allocator->hFile)) {
        return ALLOCATOR_UNABLE_EXTEND;
    }
    if (fill_mapping(allocator) != ALLOCATOR_SUCCESS) {
        return ALLOCATOR_UNABLE_MAP;
    }
    allocator->liFileSize.QuadPart = new_size;
    for (offset_t offset = old_size; offset < new_size; offset += BLOCK_SIZE) {
        if (allocator_return_block(allocator, offset) != ALLOCATOR_SUCCESS) {
            return ALLOCATOR_UNABLE_EXTEND;
        }
    }
    return ALLOCATOR_SUCCESS;
}

block *allocator_get_block(allocator *allocator) {
    if (allocator->header->free_blocks_count == 0) {
        if (allocator_reserve_blocks(allocator, 1) != ALLOCATOR_SUCCESS) {
            return NULL;
        }
    }
    offset_t next_block_offset = allocator->header->free_blocks_next;
    block *free_block = allocator_map_block(allocator, next_block_offset);
    if (free_block == NULL) {
        return NULL;
    }
    allocator->header->free_blocks_count -= 1;
    allocator->header->free_blocks_next = ((free_block_h *) free_block->ptr)->next;
    return free_block;
}

#else

#include <sys/mman.h>
#include <stddef.h>
#include <unistd.h>
#include <malloc.h>
#include <sys/stat.h>
#include <fcntl.h>

struct allocator {
    int fd;
    file_h* header;
    off_t file_size;
};

struct block {
    void *ptr;
    offset_t offset;
};

offset_t block_offset(block *block) {
    return block->offset;
}

void *block_ptr(block *block) {
    return block->ptr;
}

file_status allocator_init(file_settings *settings, allocator **allocator_ptr) {
    int fd;
    switch (settings->open_type) {
        case FILE_OPEN_EXIST: {
            fd = open(settings->path, O_RDWR);
            if (fd == -1) {
                return FILE_ST_NOT_EXIST;
            }
            break;
        }
        case FILE_OPEN_CLEAR: {
            fd = open(settings->path, O_RDWR | O_CREAT);
            if (fd == -1) {
                return FILE_ST_UNABLE_OPEN;
            }
            break;
        }
        case FILE_OPEN_CREATE: {
            fd = open(settings->path, O_RDWR | O_CREAT | O_EXCL);
            if (fd == -1) {
                return FILE_ST_ALREADY_EXISTS;
            }
            break;
        }
    }
    file_h* header;
    struct stat statbuf;
    int err = fstat(fd, &statbuf);
    if (err < 0) {
        return FILE_ST_WRONG_FORMAT;
    }
    switch (settings->open_type) {
        case FILE_OPEN_EXIST: {
            if (statbuf.st_size < BLOCK_SIZE) {
                return FILE_ST_WRONG_FORMAT;
            }
            header = mmap(NULL, BLOCK_SIZE, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
            if (header == MAP_FAILED) {
                return FILE_ST_ERROR;
            }
            if (!header_is_valid(header)) {
                return FILE_ST_WRONG_FORMAT;
            }
            break;
        }
        case FILE_OPEN_CREATE:
        case FILE_OPEN_CLEAR: {
            err = ftruncate(fd, BLOCK_SIZE);
            if (err < 0) {
                return FILE_ST_ERROR;
            }
            header = mmap(NULL, BLOCK_SIZE, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
            if (header == MAP_FAILED) {
                return FILE_ST_ERROR;
            }
            *header = (file_h) {0};
            header->magic = MAGIC;
            break;
        }
    }
    *allocator_ptr = malloc(BLOCK_SIZE);
    if (allocator_ptr == NULL) {
        return FILE_ST_ERROR;
    }
    (*allocator_ptr)->header = header;
    (*allocator_ptr)->fd = fd;
    (*allocator_ptr)->file_size = statbuf.st_size;
    return FILE_ST_OK;
}

file_status allocator_free(allocator *allocator) {
    if (munmap(allocator->header, BLOCK_SIZE) != 0) {
        return FILE_ST_UNABLE_RELEASE;
    }
    if (close(allocator->fd) != 0) {
        return FILE_ST_UNABLE_RELEASE;
    }
    free(allocator);
    return FILE_ST_OK;
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

allocator_result allocator_return_block(allocator *allocator, offset_t offset) {
    block *block = allocator_map_block(allocator, offset);
    if (block == NULL) {
        return ALLOCATOR_UNABLE_MAP;
    }
    free_block_h *free_block = block->ptr;
    free_block->next = allocator->header->free_blocks_next;
    allocator->header->free_blocks_next = offset;
    if (allocator_unmap_block(allocator, block) != ALLOCATOR_SUCCESS) {
        return ALLOCATOR_UNABLE_UNMAP;
    }
    allocator->header->free_blocks_count += 1;
    return ALLOCATOR_SUCCESS;
}

allocator_result allocator_reserve_blocks(allocator *allocator, uint32_t n) {
    if (allocator->header->free_blocks_count >= n) {
        return ALLOCATOR_SUCCESS;
    }
    uint32_t added = n - allocator->header->free_blocks_count;
    offset_t old_size = allocator->file_size;
    offset_t new_size = old_size + (BLOCK_SIZE * added);
    if (ftruncate(allocator->fd, new_size) != 0) {
        return ALLOCATOR_UNABLE_EXTEND;
    }
    allocator->file_size = new_size;
    allocator->header->free_blocks_count += added;
    for (offset_t offset = old_size; offset < new_size; offset += BLOCK_SIZE) {
        if (allocator_return_block(allocator, offset) != ALLOCATOR_SUCCESS) {
            return ALLOCATOR_UNABLE_EXTEND;
        }
    }
    return ALLOCATOR_SUCCESS;
}

block *allocator_get_block(allocator *allocator) {
    if (allocator->header->free_blocks_count == 0) {
        if (allocator_reserve_blocks(allocator, 1) != ALLOCATOR_SUCCESS) {
            return NULL;
        }
    }
    offset_t next_block_offset = allocator->header->free_blocks_next;
    block *free_block = allocator_map_block(allocator, next_block_offset);
    if (free_block == NULL) {
        return NULL;
    }
    allocator->header->free_blocks_count -= 1;
    allocator->header->free_blocks_next = ((free_block_h *) free_block->ptr)->next;
    return free_block;
}

#endif
