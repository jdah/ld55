#pragma once

#include <stdint.h>   /* IWYU pragma: export */
#include <inttypes.h> /* IWYU pragma: export */
#include <limits.h>   /* IWYU pragma: export */
#include <string.h>   /* IWYU pragma: export */

// allow for forward decls of DYNLIST types
#ifndef DYNLIST
    #define DYNLIST(_T) typeof((_T*)(NULL))
#endif // ifndef DYNLIST

// util/map.h
typedef struct map_entry map_entry_t;
typedef struct map map_t;

// util/range.h
typedef struct range range_t;

// length of compile time known array
#define ARRLEN(_arr) ((sizeof((_arr))) / ((sizeof((_arr)[0]))))

// get index of pointer in array
#define ARR_PTR_INDEX(_arr, _p) ((int) ((_p) - &(_arr)[0]))

// basic fixed-width numeric types
typedef uint8_t  u8;
typedef uint16_t u16;
typedef uint32_t u32;
typedef uint64_t u64;
typedef uintptr_t usize;
typedef int8_t  i8;
typedef int16_t i16;
typedef int32_t i32;
typedef int64_t i64;
typedef intptr_t isize;
typedef float f32;
typedef double f64;
typedef unsigned int uint;

typedef u64 hash_t;

#define U8_MIN UINT8_MIN
#define U16_MIN UINT16_MIN
#define U32_MIN UINT32_MIN
#define U64_MIN UINT64_MIN
#define U8_MAX UINT8_MAX
#define U16_MAX UINT16_MAX
#define U32_MAX UINT32_MAX
#define U64_MAX UINT64_MAX

#define USIZE_MIN UINTPTR_MIN
#define USIZE_MAX UINTPTR_MAX

#define I8_MIN INT8_MIN
#define I16_MIN INT16_MIN
#define I32_MIN INT32_MIN
#define I64_MIN INT64_MIN
#define I8_MAX INT8_MAX
#define I16_MAX INT16_MAX
#define I32_MAX INT32_MAX
#define I64_MAX INT64_MAX

#define ISIZE_MIN INTPTR_MIN
#define ISIZE_MAX INTPTR_MAX

#define PRIusize PRIuPTR
#define PRIisize PRIiPTR

#define PRIhash PRIu64

#define PRIint "d"

#define PRIf32 "f"
#define SCNf32 "f"

#define PRIf64 "f"
#define SCNf64 "lf"

// linalg types
typedef struct boxi boxi_t;
typedef struct boxf boxf_t;
typedef struct box3f box3f_t;
typedef union ivec2s ivec2s;
typedef union ivec3s ivec3s;
typedef union ivec4s ivec4s;
typedef union vec2s vec2s;
typedef union vec3s vec3s;
typedef union vec4s vec4s;
typedef union mat2s mat2s;
typedef union mat3s mat3s;
typedef union mat4s mat4s;

// aliases
typedef ivec2s v2i;
typedef ivec3s v3i;
typedef ivec4s v4i;
typedef vec2s v2;
typedef vec3s v3;
typedef vec4s v4;
typedef mat3s m3;
typedef mat4s m4;
typedef mat2s m2;

// util/alloc.h
typedef struct allocator allocator_t;

// util/any.h
typedef struct any any_t;

// types which need to be lifted out of their respective source files :)

// see util/kvstore.h
// fancy map wrapper for map of name -> any with helper functions and easy
// serialization/deserialization
typedef struct kvstore {
    // map is heap-allocated so that empty kvstores are only 2 * sizeof(void*)'

    // allocator
    allocator_t *allocator;

    // char *name -> any_t
    map_t *map;
} kvstore_t;
