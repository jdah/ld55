#pragma once

// usage: STATIC_ASSERT(<expr>, <message>)
#if __STDC_VERSION__ >= 202311L && !defined(EMSCRIPTEN)
    #define STATIC_ASSERT static_assert
#else
    #define STATIC_ASSERT _Static_assert
#endif

#include <stdint.h>

#if UINTPTR_MAX == 0xffffffff
    #define M_BITS_32
    #define M_BITS 32
    #define MAX_ALIGN 16
#elif UINTPTR_MAX == 0xffffffffffffffff
    #define M_BITS_64
    #define M_BITS 64
    #define MAX_ALIGN 16
#else
    #error wat
#endif

#ifdef EMSCRIPTEN
    STATIC_ASSERT(M_BITS == 32);
#endif

// convert preprocessor value x to string
#define _STRINGIFY_IMPL(x) #x
#define STRINGIFY(x) _STRINGIFY_IMPL(x)

// cross-platform pragma
#define PRAGMA(x) _Pragma(#x)

// concat two preprocessor variables
#define CONCAT(a, b) CONCAT_INNER(a, b)
#define CONCAT_INNER(a, b) a ## b

// loop unroll
#if defined(__clang__)
    #define UNROLL(n) PRAGMA(clang loop unroll_count(n))
#elif defined(__GNU__)
    #define UNROLL(n) PRAGMA(GCC unroll n)
#else
    #define UNROLL(n) PRAGMA(unroll)
#endif

// type "const t" -> "t"
#define unconst(t) typeof(({ t x_; __auto_type y_ = x_; y_; }))

// get type of field _f on type _t
#define typeof_field(_t, _f) typeof(((_t*) (NULL))->_f)

// get type of pointer to T
#define typeof_pointer_to(T) typeof((&((T) {})))

// get type of *T
#define typeof_deref(T) typeof(*((T)(NULL)))

// get size of field _f on type _t
#define sizeof_field(_t, _f) sizeof(((_t*) NULL)->_f)

#define M_UNUSED __attribute__((unused))
#define M_PACKED __attribute__((packed))

// force inlining
#define M_INLINE static inline __attribute__((always_inline)) M_UNUSED

// generates a warning if not the same type
#define CHECK_TYPE(_T, _a) ((void) (((typeof(_a)*) (NULL)) == (_T*)(NULL)))

// generates error if field does not exist
#define CHECK_FIELD(_T, _f) ((void) ((_T) {})._f)

// see stackoverflow.com/questions/11761703
// get number of arguments with NARG
#define NARG_SEQ()                            \
     63,62,61,60,                             \
     59,58,57,56,55,54,53,52,51,50,           \
     49,48,47,46,45,44,43,42,41,40,           \
     39,38,37,36,35,34,33,32,31,30,           \
     29,28,27,26,25,24,23,22,21,20,           \
     19,18,17,16,15,14,13,12,11,10,           \
     9,8,7,6,5,4,3,2,1,0
#define NARG_N(                               \
     _1, _2, _3, _4, _5, _6, _7, _8, _9,_10,  \
     _11,_12,_13,_14,_15,_16,_17,_18,_19,_20, \
     _21,_22,_23,_24,_25,_26,_27,_28,_29,_30, \
     _31,_32,_33,_34,_35,_36,_37,_38,_39,_40, \
     _41,_42,_43,_44,_45,_46,_47,_48,_49,_50, \
     _51,_52,_53,_54,_55,_56,_57,_58,_59,_60, \
     _61,_62,_63,N,...) N
#define NARG_I(...) NARG_N(__VA_ARGS__)
#define NARG(...) NARG_I(__VA_ARGS__, NARG_SEQ())

// general definition for any function name
#define _VMACRO(name, n) CONCAT(name, n)
#define VMACRO(func, ...) _VMACRO(func, NARG(__VA_ARGS__))(__VA_ARGS__)

#define _NTH_ARG0(A0, ...) A0
#define _NTH_ARG1(A0, A1, ...) A1
#define _NTH_ARG2(A0, A1, A2, ...) A2
#define _NTH_ARG3(A0, A1, A2, A3, ...) A3
#define _NTH_ARG4(A0, A1, A2, A3, A4, ...) A4
#define _NTH_ARG5(A0, A1, A2, A3, A4, A5, ...) A5
#define _NTH_ARG6(A0, A1, A2, A3, A4, A5, A6, ...) A6
#define _NTH_ARG7(A0, A1, A2, A3, A4, A5, A6, A7, ...) A7
#define _NTH_ARG8(A0, A1, A2, A3, A4, A5, A6, A7, A8, ...) A8

// get NTH_ARG
#define _NTH_ARG(N) CONCAT(_NTH_ARG, N)
#define NTH_ARG(N, ...) _NTH_ARG(NARG(__VA_ARGS__))(__VA_ARGS__)

#ifdef EMSCRIPTEN
    #define htonll(x_) ({ typeof(x_) x = (x_); (((uint64_t)htonl((uint64_t) x)) << 32) + htonl(((uint64_t) (x)) >> 32); })
    #define ntohll(x_) ({ typeof(x_) x = (x_); (((uint64_t)ntohl((uint64_t) x)) << 32) + ntohl(((uint64_t) (x)) >> 32); })
#endif // ifdef EMSCRIPTEN
