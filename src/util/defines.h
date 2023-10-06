//
// Created by vyach on 04.10.2023.
//

#ifndef LLP_LAB1_DEFINES_H
#define LLP_LAB1_DEFINES_H

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
