//
// Created by vyach on 15.09.2023.
//

#ifndef LLP_LAB1_FILE_H
#define LLP_LAB1_FILE_H

#include <stdint.h>
#include "allocator/allocator.h"

typedef struct {
    int32_t magic;
    uint32_t free_blocks_count;
    offset_t free_blocks_next;
    // entry point
} file_header;

//void open_file(char *file_name) {
//    HANDLE file_handle;
//    LARGE_INTEGER fileSize;
//    file_handle = CreateFile(file_name,
//                             GENERIC_READ | GENERIC_WRITE,
//                             FILE_SHARE_READ | FILE_SHARE_WRITE,
//                             NULL,
//                             OPEN_ALWAYS,
//                             FILE_ATTRIBUTE_NORMAL,
//                             0);
//    if (file_handle == INVALID_HANDLE_VALUE) {
//        exit(-1);
//    }
//    if (!GetFileSizeEx(file_handle, &fileSize)) {
//        CloseHandle(file_handle);
//        exit(-1);
//    }
//    CloseHandle(file_handle);
//}

#endif //LLP_LAB1_FILE_H
