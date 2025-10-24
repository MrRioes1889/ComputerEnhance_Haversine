#pragma once
#include <stdint.h>

typedef int8_t int8;
typedef int16_t int16;
typedef int32_t int32;
typedef int64_t int64;
typedef uint8_t uint8;
typedef uint16_t uint16;
typedef uint32_t uint32;
typedef uint64_t uint64;
typedef float float32;
typedef double float64;
typedef uint8_t bool8;

typedef uint8_t Byte;
typedef uint16_t Word;

#ifdef _MSC_VER
#pragma warning(disable : 4100)
#pragma warning(disable : 4101)
#pragma warning(disable : 4189)
#define unreachable() __assume(0)

#else
#define unreachable() __builtin_unreachable()
#endif

#define false 0
#define true 1
#define array_count(arr) (sizeof(arr) / sizeof(arr[0]))