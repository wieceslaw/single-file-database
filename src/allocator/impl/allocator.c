//
// Created by vyach on 15.09.2023.
//

#include <stdbool.h>
#include <stddef.h>
#include <malloc.h>
#include <stdio.h>
#include "../allocator.h"
#include "blocklist.h"

typedef struct free_block {
    offset_t next;
} free_block_h;

static bool header_is_valid(file_h *header) {
    return MAGIC == header->magic;
}

static offset_t granularity = 0;

static offset_t sys_granularity();

static offset_t allocation_granularity() {
    if (granularity == 0) {
        granularity = sys_granularity();
    }
    return granularity;
}

static offset_t granular_offset(offset_t offset) {
    if (granularity == 0) {
        granularity = allocation_granularity();
    }
    return (offset / granularity) * granularity;
}

static allocator_result unmap_block(block *block);

static block *map_block(allocator *allocator, offset_t offset);

static block_list *allocator_get_list(allocator *allocator);

static file_h *allocator_get_header(allocator *allocator);

static allocator_result allocator_clear_list(allocator *allocator) {
    block_list_it it;
    block_list_iterator(allocator_get_list(allocator), &it);
    while (!block_list_iterator_is_empty(&it)) {
        block_list_node *node = it.node;
        block_list_iterator_next(&it);
        if (unmap_block(node->block) != ALLOCATOR_SUCCESS) {
            return ALLOCATOR_UNABLE_UNMAP;
        }
        if (!block_list_delete(allocator_get_list(allocator), node)) {
            return ALLOCATOR_UNABLE_UNMAP;
        }
    }
    return ALLOCATOR_SUCCESS;
}

static block_list_node *block_list_find(block_list *list, offset_t offset) {
    block_list_it it;
    block_list_iterator(list, &it);
    while (!block_list_iterator_is_empty(&it)) {
        if (it.node->file_offset == offset) {
            return it.node;
        }
        block_list_iterator_next(&it);
    }
    return NULL;
}

void *block_ref_ptr(block_ref *ref) {
    return ref->block->ptr + ref->offset;
}

block_ref *allocator_map_block_ref(allocator *allocator, offset_t offset) {
    block_list *list = allocator_get_list(allocator);
    offset_t fixed_offset = granular_offset(offset);
    block_list_node *node = block_list_find(list, fixed_offset);
    block_ref *ref = malloc(sizeof(block_ref));
    if (ref == NULL) {
        return NULL;
    }
    if (node == NULL) {
        block *block = map_block(allocator, fixed_offset);
        if (block == NULL) {
            free(ref);
            return NULL;
        }
        node = block_list_append(list, block);
        if (NULL == node) {
            free(ref);
            return NULL;
        }
    }
    node->used++;
    *ref = (block_ref) {.offset = offset - fixed_offset, .block = node->block};
    return ref;
}

allocator_result allocator_unmap_block_ref(allocator *allocator, block_ref *ref) {
    if (NULL == allocator || NULL == ref) {
        return ALLOCATOR_UNABLE_UNMAP;
    }
    block_list *list = allocator_get_list(allocator);
    block_list_node *node = block_list_find(list, ref->block->file_offset);
    if (NULL == node) {
        return ALLOCATOR_UNABLE_UNMAP;
    }
    node->used -= 1;
    if (0 == node->used) {
        if (0 != unmap_block(ref->block)) {
            block_list_delete(list, node);
            free(ref->block);
            free(ref);
            return ALLOCATOR_UNABLE_UNMAP;
        }
        if (!block_list_delete(list, node)) {
            free(ref->block);
            free(ref);
            return ALLOCATOR_UNABLE_UNMAP;
        }
    }
    free(ref);
    return ALLOCATOR_SUCCESS;
}

allocator_result allocator_return_block(allocator *allocator, offset_t offset) {
    block_ref *ref = allocator_map_block_ref(allocator, offset);
    if (NULL == ref) {
        return ALLOCATOR_UNABLE_MAP;
    }
    free_block_h *free_block = block_ref_ptr(ref);
    file_h *header = allocator_get_header(allocator);
    free_block->next = header->free_blocks_next;
    header->free_blocks_next = offset;
    if (ALLOCATOR_SUCCESS != allocator_unmap_block_ref(allocator, ref)) {
        return ALLOCATOR_UNABLE_UNMAP;
    }
    header->free_blocks_count += 1;
    return ALLOCATOR_SUCCESS;
}

block_ref *allocator_get_block_ref(allocator *allocator) {
    if (allocator_get_header(allocator)->free_blocks_count == 0) {
        if (ALLOCATOR_SUCCESS != allocator_reserve_blocks(allocator, 1)) {
            return NULL;
        }
    }
    offset_t next_block_offset = allocator_get_header(allocator)->free_blocks_next;
    block_ref *ref = allocator_map_block_ref(allocator, next_block_offset);
    if (NULL == ref) {
        return NULL;
    }
    allocator_get_header(allocator)->free_blocks_count -= 1;
    allocator_get_header(allocator)->free_blocks_next = ((free_block_h *) block_ref_ptr(ref))->next;
    return ref;
}

#ifdef _WIN32

#include "windows.h"

struct allocator {
    HANDLE hFile;
    HANDLE hMap;
    LARGE_INTEGER liFileSize;
    block_list block_list;
    block_ref *header_ref;
};

offset_t sys_granularity() {
    SYSTEM_INFO sys_info;
    GetSystemInfo(&sys_info);
    return sys_info.dwAllocationGranularity;
}

static allocator_result unmap_block(block *block) {
    if (block == NULL) {
        return ALLOCATOR_UNABLE_UNMAP;
    }
    if (!UnmapViewOfFile(block->ptr)) {
        free(block);
        return ALLOCATOR_UNABLE_UNMAP;
    }
    free(block);
    return ALLOCATOR_SUCCESS;
}

static block *map_block(allocator *allocator, offset_t file_offset) {
    ULARGE_INTEGER ulOffset;
    ulOffset.QuadPart = file_offset;
    offset_t mapping_size = min(allocation_granularity(), allocator->liFileSize.QuadPart - file_offset);
    void *ptr = MapViewOfFile(
            allocator->hMap,
            FILE_MAP_READ | FILE_MAP_WRITE,
            ulOffset.HighPart,
            ulOffset.LowPart,
            mapping_size);
    if (ptr == NULL) {
        printf("%lu", GetLastError());
        return NULL;
    }
    block *block_ptr = malloc(sizeof(block));
    if (block_ptr == NULL) {
        UnmapViewOfFile(ptr);
        return NULL;
    }
    block_ptr->file_offset = file_offset;
    block_ptr->ptr = ptr;
    return block_ptr;
}

static block_list *allocator_get_list(allocator *allocator) {
    return &(allocator->block_list);
}

static file_h *allocator_get_header(allocator *allocator) {
    return block_ref_ptr(allocator->header_ref);
}

file_status allocator_init(file_settings *settings, allocator **allocator_ptr) {
    *allocator_ptr = malloc(sizeof(allocator));
    if (*allocator_ptr == NULL) {
        return FILE_ST_ERROR;
    }
    **allocator_ptr = (allocator) {0};
    allocator *allocator = *allocator_ptr;
    switch (settings->open_type) {
        case FILE_OPEN_EXIST: {
            allocator->hFile = CreateFile(settings->path,
                                                 GENERIC_READ | GENERIC_WRITE,
                                                 FILE_SHARE_READ | FILE_SHARE_WRITE,
                                                 NULL,
                                                 OPEN_EXISTING,
                                                 FILE_ATTRIBUTE_NORMAL,
                                                 0);
            if (INVALID_HANDLE_VALUE == allocator->hFile) {
                return FILE_ST_ALREADY_EXISTS;
            }
            break;
        }
        case FILE_OPEN_CLEAR: {
            allocator->hFile = CreateFile(settings->path,
                                                 GENERIC_READ | GENERIC_WRITE,
                                                 FILE_SHARE_READ | FILE_SHARE_WRITE,
                                                 NULL,
                                                 OPEN_ALWAYS,
                                                 FILE_ATTRIBUTE_NORMAL,
                                                 0);
            if (INVALID_HANDLE_VALUE == allocator->hFile) {
                return FILE_ST_UNABLE_OPEN;
            }
            break;
        }
        case FILE_OPEN_CREATE: {
            allocator->hFile = CreateFile(settings->path,
                                                 GENERIC_READ | GENERIC_WRITE,
                                                 FILE_SHARE_READ | FILE_SHARE_WRITE,
                                                 NULL,
                                                 CREATE_NEW,
                                                 FILE_ATTRIBUTE_NORMAL,
                                                 0);
            if (INVALID_HANDLE_VALUE == allocator->hFile) {
                return FILE_ST_ALREADY_EXISTS;
            }
            break;
        }
    }
    if (!GetFileSizeEx((*allocator_ptr)->hFile, &allocator->liFileSize)) {
        CloseHandle((*allocator_ptr)->hFile);
        return FILE_ST_ERROR;
    }
    switch (settings->open_type) {
        case FILE_OPEN_EXIST: {
            if ((*allocator_ptr)->liFileSize.QuadPart < BLOCK_SIZE) {
                CloseHandle((*allocator_ptr)->hFile);
                return FILE_ST_WRONG_FORMAT;
            }
            allocator->hMap = CreateFileMapping(
                    allocator->hFile,
                    NULL,
                    PAGE_READWRITE,
                    0,
                    0,
                    NULL);
            if (0 == allocator->hMap) {
                CloseHandle((*allocator_ptr)->hFile);
                return FILE_ST_ERROR;
            }
            allocator->header_ref = allocator_map_block_ref(allocator, 0);
            if (allocator->header_ref == NULL) {
                CloseHandle((*allocator_ptr)->hMap);
                CloseHandle((*allocator_ptr)->hFile);
                return FILE_ST_ERROR;
            }
            file_h *header = allocator_get_header(allocator);
            if (!header_is_valid(header)) {
                CloseHandle((*allocator_ptr)->hMap);
                CloseHandle((*allocator_ptr)->hFile);
                return FILE_ST_WRONG_FORMAT;
            }
            break;
        }
        case FILE_OPEN_CREATE:
        case FILE_OPEN_CLEAR: {
            offset_t new_size = allocation_granularity();
            if (INVALID_SET_FILE_POINTER ==
                SetFilePointer((*allocator_ptr)->hFile, new_size, NULL, FILE_BEGIN)) {
                return FILE_ST_ERROR;
            }
            allocator->liFileSize.QuadPart = new_size;
            if (!SetEndOfFile((*allocator_ptr)->hFile)) {
                return FILE_ST_ERROR;
            }
            allocator->hMap = CreateFileMapping(
                    allocator->hFile,
                    NULL,
                    PAGE_READWRITE,
                    0,
                    0,
                    NULL);
            if (0 == allocator->hMap) {
                CloseHandle((*allocator_ptr)->hFile);
                return FILE_ST_ERROR;
            }
            allocator->header_ref = allocator_map_block_ref(allocator, 0);
            if (allocator->header_ref == NULL) {
                CloseHandle((*allocator_ptr)->hMap);
                CloseHandle((*allocator_ptr)->hFile);
                return FILE_ST_ERROR;
            }
            file_h *header = allocator_get_header(*allocator_ptr);
            *header = (file_h) {0};
            header->magic = MAGIC;
            for (offset_t file_offset = BLOCK_SIZE; file_offset < new_size; file_offset += BLOCK_SIZE) {
                if (ALLOCATOR_SUCCESS != allocator_return_block(allocator, file_offset)) {
                    return FILE_ST_ERROR;
                }
            }
            break;
        }
    }
    return FILE_ST_OK;
}

file_status allocator_free(allocator *allocator) {
    if (allocator_unmap_block_ref(allocator, allocator->header_ref) != ALLOCATOR_SUCCESS) {
        allocator_clear_list(allocator);
        CloseHandle(allocator->hMap);
        CloseHandle(allocator->hFile);
        free(allocator);
        return FILE_ST_UNABLE_RELEASE;
    }
    if (allocator_clear_list(allocator) != ALLOCATOR_SUCCESS) {
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

static allocator_result clear_mapping(allocator *allocator) {
    block_list_it it;
    block_list_iterator(&allocator->block_list, &it);
    while (!block_list_iterator_is_empty(&it)) {
        block *block = block_list_iterator_get(&it);
        if (!FlushViewOfFile(block->ptr, 0)) {
            return ALLOCATOR_UNABLE_EXTEND;
        }
        if (!UnmapViewOfFile(block->ptr)) {
            return ALLOCATOR_UNABLE_EXTEND;
        }
        free(block);
        it.node->block = NULL;
        block_list_iterator_next(&it);
    }
    if (!FlushFileBuffers(allocator->hFile)) {
        return ALLOCATOR_UNABLE_EXTEND;
    }
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
        block *block_ptr = map_block(allocator, it.node->file_offset);
        if (block_ptr == NULL) {
            return ALLOCATOR_UNABLE_MAP;
        }
        it.node->block = block_ptr;
        block_list_iterator_next(&it);
    }
    return ALLOCATOR_SUCCESS;
}

allocator_result allocator_reserve_blocks(allocator *allocator, uint32_t n) {
    file_h *header = allocator_get_header(allocator);
    uint32_t count = header->free_blocks_count;
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
    for (offset_t file_offset = old_size; file_offset < new_size; file_offset += BLOCK_SIZE) {
        if (allocator_return_block(allocator, file_offset) != ALLOCATOR_SUCCESS) {
            return ALLOCATOR_UNABLE_EXTEND;
        }
    }
    return ALLOCATOR_SUCCESS;
}

#else

#include <sys/mman.h>
#include <unistd.h>
#include <sys/stat.h>
#include <fcntl.h>

struct allocator {
    int fd;
    block_ref *header_ref;
    offset_t file_size;
    block_list list;
};

static offset_t sys_granularity() {
    return getpagesize();
}

static allocator_result unmap_block(block *block) {
    if (munmap(block->ptr, granularity) == 0) {
        return ALLOCATOR_SUCCESS;
    }
    return ALLOCATOR_UNABLE_UNMAP;
}

static block *map_block(allocator *allocator, offset_t offset) {
    void *block_mapping = mmap(NULL, allocation_granularity(), PROT_READ | PROT_WRITE, MAP_SHARED, allocator->fd, offset);
    if (MAP_FAILED == block_mapping) {
        return NULL;
    }
    block *block_ptr = malloc(sizeof(block));
    if (NULL == block_ptr) {
        munmap(block_mapping, BLOCK_SIZE);
        return NULL;
    }
    block_ptr->ptr = block_mapping;
    block_ptr->file_offset = offset;
    return block_ptr;
}

static block_list *allocator_get_list(allocator *allocator) {
    return &(allocator->list);
}

static file_h *allocator_get_header(allocator *allocator) {
    return block_ref_ptr(allocator->header_ref);
}

file_status allocator_init(file_settings *settings, allocator **allocator_ptr) {
    *allocator_ptr = malloc(BLOCK_SIZE);
    if (NULL == allocator_ptr) {
        return FILE_ST_ERROR;
    }
    **allocator_ptr = (allocator) {0};
    allocator* allocator = *allocator_ptr;
    switch (settings->open_type) {
        case FILE_OPEN_EXIST: {
            allocator->fd = open(settings->path, O_RDWR);
            if (-1 == allocator->fd) {
                return FILE_ST_NOT_EXIST;
            }
            break;
        }
        case FILE_OPEN_CLEAR: {
            allocator->fd = open(settings->path, O_RDWR | O_CREAT);
            if (-1 == allocator->fd) {
                return FILE_ST_UNABLE_OPEN;
            }
            break;
        }
        case FILE_OPEN_CREATE: {
            allocator->fd = open(settings->path, O_RDWR | O_CREAT | O_EXCL);
            if (-1 == allocator->fd) {
                return FILE_ST_ALREADY_EXISTS;
            }
            break;
        }
    }
    struct stat statbuf;
    int err = fstat(allocator->fd, &statbuf);
    if (err < 0) {
        return FILE_ST_WRONG_FORMAT;
    }
    allocator->file_size = statbuf.st_size;
    switch (settings->open_type) {
        case FILE_OPEN_EXIST: {
            if (allocator->file_size < BLOCK_SIZE) {
                return FILE_ST_WRONG_FORMAT;
            }
            allocator->header_ref = allocator_map_block_ref(allocator, 0);
            if (allocator->header_ref == NULL) {
                return FILE_ST_ERROR;
            }
            if (!header_is_valid(allocator_get_header(allocator))) {
                allocator_unmap_block_ref(allocator, allocator->header_ref);
                return FILE_ST_WRONG_FORMAT;
            }
            break;
        }
        case FILE_OPEN_CREATE:
        case FILE_OPEN_CLEAR: {
            offset_t new_size = allocation_granularity();
            err = ftruncate(allocator->fd, new_size);
            allocator->file_size = new_size;
            if (err < 0) {
                return FILE_ST_ERROR;
            }
            allocator->header_ref = allocator_map_block_ref(allocator, 0);
            if (allocator->header_ref == NULL) {
                return FILE_ST_ERROR;
            }
            *allocator_get_header(allocator) = (file_h) {0};
            allocator_get_header(allocator)->magic = MAGIC;
            for (offset_t offset = BLOCK_SIZE; offset < new_size; offset += BLOCK_SIZE) {
                if (ALLOCATOR_SUCCESS != allocator_return_block(allocator, offset)) {
                    return FILE_ST_ERROR;
                }
            }
            break;
        }
    }
    return FILE_ST_OK;
}

file_status allocator_free(allocator *allocator) {
    if (ALLOCATOR_SUCCESS != allocator_unmap_block_ref(allocator, allocator->header_ref)) {
        allocator_clear_list(allocator);
        close(allocator->fd);
        free(allocator);
        return FILE_ST_UNABLE_RELEASE;
    }
    if (ALLOCATOR_SUCCESS != allocator_clear_list(allocator)) {
        close(allocator->fd);
        free(allocator);
        return FILE_ST_UNABLE_RELEASE;
    }
    if (0 != close(allocator->fd)) {
        free(allocator);
        return FILE_ST_UNABLE_RELEASE;
    }
    free(allocator);
    return FILE_ST_OK;
}

allocator_result allocator_reserve_blocks(allocator *allocator, uint32_t n) {
    if (allocator_get_header(allocator)->free_blocks_count >= n) {
        return ALLOCATOR_SUCCESS;
    }
    uint32_t added = n - allocator_get_header(allocator)->free_blocks_count;
    offset_t old_size = allocator->file_size;
    offset_t new_size = old_size + (BLOCK_SIZE * added);
    if (ftruncate(allocator->fd, new_size) != 0) {
        return ALLOCATOR_UNABLE_EXTEND;
    }
    allocator->file_size = new_size;
    for (offset_t offset = old_size; offset < new_size; offset += BLOCK_SIZE) {
        if (ALLOCATOR_SUCCESS != allocator_return_block(allocator, offset)) {
            return ALLOCATOR_UNABLE_EXTEND;
        }
    }
    return ALLOCATOR_SUCCESS;
}

#endif
