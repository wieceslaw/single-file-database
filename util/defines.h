//
// Created by vyach on 04.10.2023.
//

#ifndef LLP_LAB1_DEFINES_H
#define LLP_LAB1_DEFINES_H

#ifndef __FILE_NAME__
#define __FILE_NAME__ (__builtin_strrchr(__FILE__, '/') ? __builtin_strrchr(__FILE__, '/') + 1 : __FILE__)
#endif

#if defined NDEBUG
#define debug( format, ... )
#else
#define debug(format, ...) printf("%s::%s::%d " format "\n", __FILE_NAME__, __func__, __LINE__, ##__VA_ARGS__)
#endif

#define loginfo(msg_, ...) fprintf(stdout, "[INFO]: " msg_ "\n", ##__VA_ARGS__)
#define logerr(msg_, ...) fprintf(stderr, "[ERROR]: " msg_ "\n", ##__VA_ARGS__)


#define MAX(a,b) (((a)>(b))?(a):(b))
#define MIN(a, b) (((a)<(b))?(a):(b))

#ifdef __GNUC__
#include <stdint-gcc.h>
#define PACK(__Declaration__) __Declaration__ __attribute__((__packed__))
#endif

#ifdef _MSC_VER
typedef __int32 int32_t;
typedef unsigned __int32 uint32_t;
typedef __int64 int64_t;
typedef unsigned __int64 uint64_t;
typedef unsigned __int8 uint8_t;

#define PACK( __Declaration__ ) __pragma( pack(push, 1) ) __Declaration__ __pragma( pack(pop))
#endif

#endif //LLP_LAB1_DEFINES_H
