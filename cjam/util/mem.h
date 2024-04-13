#pragma once

#include "types.h"
#include "macros.h"

// swap _a and _b
#define swap(_a, _b) ({ typeof(_a) _x = (_a); _a = (_b); _b = (_x); })

// find offset of bitfield in struct
// DO NOT USE IN PERFORMANT CODE
#define bitoffsetof(t, f) ({                                                   \
    union { u8 bytes[sizeof(t)]; t typ; } u;                                   \
	memset(&u, 0, sizeof(u));                                                  \
    ++u.typ.f;                                                                 \
    bitmap_t b = { .size = sizeof(t) * 8, .bits = u.bytes };                   \
    bitmap_find(&b, 0, true); })

M_INLINE void memset32(void *dst, u32 val, usize n) {
    u32 *dst32 = dst;
    while (n--) { *dst32++ = val; }
}

M_INLINE void memset16(void *dst, u16 val, usize n) {
    u16 *dst16 = dst;
    while (n--) { *dst16++ = val; }
}

M_INLINE void memsetf64(void *dst, f64 val, usize n) {
    f64 *dst64 = dst;
    while (n--) { *dst64++ = val; }
}

M_INLINE void memsetf32(void *dst, f32 val, usize n) {
    f32 *dst32 = dst;
    while (n--) { *dst32++ = val; }
}

M_INLINE int memvcmp(const void *ptr, u8 v, usize n) {
    if (n == 0) { return 1; }

    if (*((u8*) ptr) != v) {
        return *((u8*) ptr) - v;
    } else if (n > 1) {
        return memcmp(ptr, ptr + 1, n - 1);
    }

    return 0;
}

// NOTE: memory must be aligned!
M_INLINE void memswap(void *a_, void *b_, usize n) {
    if (n == 1) {
        swap(*((u8*) a_), *((u8*) b_));
    } else if (n == 2) {
        swap(*((u16*) a_), *((u16*) b_));
    } else if (n == 4) {
        swap(*((u32*) a_), *((u32*) b_));
    } else if (n == 8) {
        swap(*((u64*) a_), *((u64*) b_));
    } else {
        u8 *a = a_, *b = b_, *a_end = a + n;

        while (a < a_end) {
            swap(*a, *b);
            a++;
            b++;
        }
    }
}
