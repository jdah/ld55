#pragma once

#include <stdlib.h>

#include "assert.h"
#include "macros.h"
#include "types.h"
#include "math.h"
#include "alloc.h"

typedef struct bitmap {
    // allocator, NULL indicates a stack-allocated bitmap
    allocator_t *allocator;

    // size in bits
    int size;

    // data - typically points directly after struct header, but necessary
    // as a bitmap_t with a flexible array member could not be stored as a
    // struct field
    u8 *bits;

    // TODO: evt. userdata - used to enforce 16-byte alignment of data
    void *padding;
} bitmap_t;

// _sz number of bits -> bytes
#define BITMAP_SIZE_TO_BYTES(_sz) (((_sz) + 7) / 8)

// excess bits on bitmap of size _sz, i.e. size 11 = 2 bytes -> 5 extra bits
#define BITMAP_EXCESS_BITS(_sz) (BITMAP_SIZE_TO_BYTES((_sz)) * 8 - (_sz))

#define BITMAP_STACKALLOC_IMPL(_name, _sz, _szname)                            \
    const size_t _szname = (_sz);                                              \
    bitmap_t *_name = alloca(sizeof(bitmap_t) + BITMAP_SIZE_TO_BYTES(_sz));    \
    *_name = (bitmap_t) {                                                      \
        .allocator = NULL, .size = _szname, .bits = (u8*) (_name + 1)          \
    }

#define BITMAP_STACKALLOC(_name, _sz)                                          \
    BITMAP_STACKALLOC_IMPL(_name, _sz, CONCAT(_size, __COUNTER__))

#define BITMAP_DECL_IMPL(_name, _sz, _storage)                                 \
    union {                                                                    \
        bitmap_t _name;                                                        \
        u8 _storage[sizeof(bitmap_t) + BITMAP_SIZE_TO_BYTES(_sz)];             \
    }

#define BITMAP_DECL(_name, _sz)                                                \
    BITMAP_DECL_IMPL(_name, _sz, CONCAT(_storage, __COUNTER__))

#define BITMAP_DECL_STATIC_IMPL(_name, _sz, _storage)                          \
    static u8 _storage[sizeof(bitmap_t) + BITMAP_SIZE_TO_BYTES(_sz)];          \
    static bitmap_t _name =                                                    \
        (bitmap_t) { .allocator = NULL, .size = _sz, .bits = _storage };

#define BITMAP_DECL_STATIC(_name, _sz)                                         \
    BITMAP_DECL_STATIC_IMPL(_name, _sz, CONCAT(_storage, __COUNTER__))

// TODO: disable on release builds
#define BITMAP_CHECK(_pb) ({                                                   \
        ASSERT((_pb));                                                         \
        ASSERT((_pb)->size);                                                   \
        ASSERT((_pb)->bits);                                                   \
    })

M_INLINE bool bitmap_valid(bitmap_t *b) {
    return b && b->size && b->bits;
}

M_INLINE void bitmap_destroy(bitmap_t *b) {
    BITMAP_CHECK(b);
    ASSERT(b->allocator);
    allocator_t *a = b->allocator;
    mem_free(a, b);
    *b = (bitmap_t) { 0 };
}

M_INLINE void bitmap_init(bitmap_t *b, allocator_t *a, int sz) {
    *b = (bitmap_t) {
        .allocator = a,
        .size = sz,
        .bits = (u8*) (b + 1),
    };
}

M_INLINE bitmap_t *bitmap_create(allocator_t *a, int sz) {
    bitmap_t *b = mem_alloc(a, sizeof(bitmap_t) + BITMAP_SIZE_TO_BYTES(sz));
    bitmap_init(b, a, sz);
    return b;
}

M_INLINE bitmap_t *bitmap_create_zero(allocator_t *a, int sz) {
    bitmap_t *b = mem_calloc(a, sizeof(bitmap_t) + BITMAP_SIZE_TO_BYTES(sz));
    bitmap_init(b, a, sz);
    return b;
}

M_INLINE bitmap_t *bitmap_resize(bitmap_t *b, int new_size) {
    BITMAP_CHECK(b);

    bitmap_t *c = bitmap_create(b->allocator, new_size);
    memcpy(c->bits, b->bits, BITMAP_SIZE_TO_BYTES(b->size));
    bitmap_destroy(b);
    return c;
}

M_INLINE bitmap_t *bitmap_resize_or_create(
    bitmap_t *b, allocator_t *a, int new_size) {
    return
        !b || !bitmap_valid(b) ?
            bitmap_create(a, new_size)
            : bitmap_resize(b, new_size);
}

// resize bitmap, filling new space with zero
M_INLINE bitmap_t *bitmap_resize_zero(bitmap_t *b, int new_size) {
    BITMAP_CHECK(b);

    const int
        b_old = BITMAP_SIZE_TO_BYTES(b->size),
        b_new = BITMAP_SIZE_TO_BYTES(new_size);

    bitmap_t *p = bitmap_resize(b, new_size);

    if (b_old < b_new) {
        memset(&p->bits[b_old], 0, b_new - b_old);
    }

    return p;
}

M_INLINE bitmap_t *bitmap_resize_or_create_zero(
    bitmap_t *b, allocator_t *a, int new_size) {
    return
        !b || !bitmap_valid(b) ?
            bitmap_create_zero(a, new_size)
            : bitmap_resize_zero(b, new_size);
}

M_INLINE void bitmap_copy(bitmap_t *dst, bitmap_t *src) {
    ASSERT(bitmap_valid(dst));
    ASSERT(bitmap_valid(src));
    ASSERT(dst->size >= src->size);
    ASSERT(src->size);
    ASSERT(dst->size);
    memcpy(dst->bits, src->bits, BITMAP_SIZE_TO_BYTES(src->size));
}

M_INLINE void bitmap_set(bitmap_t *b, int n) {
    b->bits[n >> 3] |= (1 << (n & 7));
}

M_INLINE bool bitmap_get(const bitmap_t *b, int n) {
    return !!(b->bits[n >> 3] & (1 << (n & 7)));
}

M_INLINE void bitmap_clr(bitmap_t *b, int n) {
    b->bits[n >> 3] &= ~(1 << (n & 7));
}

M_INLINE void bitmap_put(bitmap_t *b, int n, bool val) {
    if (val) { bitmap_set(b, n); } else { bitmap_clr(b, n); }
}

M_INLINE void bitmap_toggle(bitmap_t *b, int n) {
    b->bits[n >> 3] ^= (1 << (n & 7));
}

M_INLINE void bitmap_fill(bitmap_t *b, bool val) {
    memset(b->bits, val ? 0xFF : 0x00, BITMAP_SIZE_TO_BYTES(b->size));
}

M_INLINE void bitmap_fill_n(bitmap_t *b, bool val, int n) {
    ASSERT(n <= b->size, "%d > %d", n, b->size);

    if ((n % 8) == 0) {
        memset(b->bits, val ? 0xFF : 0x00, BITMAP_SIZE_TO_BYTES(n));
    } else {
        const int extra = n % 8;

        if (n > 8) {
            // set all bytes except the last
            memset(b->bits, val ? 0xFF : 0x00, BITMAP_SIZE_TO_BYTES(n) - 1);
        }

        // set individual bits in last byte
        for (int i = n - extra; i < n; i++) {
            bitmap_put(b, i, val);
        }
    }
}

// a |= b
M_INLINE void bitmap_or(bitmap_t *a, bitmap_t *b) {
    const int n =
        min(BITMAP_SIZE_TO_BYTES(a->size), BITMAP_SIZE_TO_BYTES(b->size));

    for (int i = 0; i < n; i++) {
        a->bits[i] |= b->bits[i];
    }
}

// a &= b
M_INLINE void bitmap_and(bitmap_t *a, bitmap_t *b) {
    const int n =
        min(BITMAP_SIZE_TO_BYTES(a->size), BITMAP_SIZE_TO_BYTES(b->size));

    for (int i = 0; i < n; i++) {
        a->bits[i] &= b->bits[i];
    }
}

// count number of bits with value
M_INLINE int bitmap_count(const bitmap_t *b, bool val) {
    BITMAP_CHECK(b);

    const u8 *p = b->bits, *end = b->bits + BITMAP_SIZE_TO_BYTES(b->size);
    int n = 0;

#define COUNT_FOR_T(_T)                                             \
    while ((end - p) >= (int) sizeof(_T)) {                         \
        const _T t = *((_T*) p);                                    \
        n += val ? popcount(t) : invpopcount(t);                    \
        p += sizeof(_T);                                            \
    }

    COUNT_FOR_T(u64)
    COUNT_FOR_T(u32)
    COUNT_FOR_T(u16)
    COUNT_FOR_T(u8)

#undef COUNT_FOR_T

    // don't count extra bits on the last byte
    if (b->size % 8 != 0) {
        // mask of extra bits on end of bitmap
        // fx. size = 11, size_bytes = 2, total size bits = 16
        //     data     | garbage
        // 00000000 000 | 01010
        // exmask = 000 | 11111
        // extra  = 000 | 01010
        const u8
            exmask = ~((1 << (8 - BITMAP_EXCESS_BITS(b->size))) - 1),
            extra = (*(end - 1) & exmask) | (val ? 0 : ~exmask);

        n -= val ? popcount(extra) : invpopcount(extra);
    }

    return n;
}

// returns index of lowest bit with value or -1 if there is no such bit
M_INLINE int bitmap_find(const bitmap_t *b, int start, bool val) {
    BITMAP_CHECK(b);

    if (start >= b->size) { return -1; }

    // naive quick check: we have to look at this byte anyway, so just pull it
    // into the cache and check it :)
    if (bitmap_get(b, start) == val) { return start; }

    const u8
        *p = b->bits + (start / 8),
        *end = b->bits + BITMAP_SIZE_TO_BYTES(b->size);

    // search start value - offset according to start if not byte aligned
    int from = start % 8;

    bool force_byte = false;

    // if we're not aligned, need to start by doing a by-byte search
    if ((start / 8) % sizeof(u64) != 0) { goto by_byte; }

by_u64:;
    // search as u64s until we hit the end OR find the u64 containing val
    // then go to byte search, but "force" it (don't go back to u64 search)
    // this version is (experimentally) ~6x faster than the raw byte search
    const u64 *p64 = (u64*) p;
    while (true) {
        const u8 *p64_next = (u8*) (p64 + 1);
        if (p64_next >= end
            || ((u64) (p64_next - end) < sizeof(u64))
            || (val ? popcount(*p64) : invpopcount(*p64)) != 0) {
            p = (u8*) p64;
            force_byte = true;
            break;
        }

        p64++;
    }

by_byte:
    while (p != end) {
        // search by u64 if we're aligned
        if (!force_byte
            && ((u64) (p - b->bits)) % sizeof(u64) == 0
            && ((u64) (end - p) >= sizeof(u64))) {
            goto by_u64;
        }

        if ((val ? popcount(*p) : invpopcount(*p)) != 0) {
            const int to =
                p == (end - 1) ? (8 - BITMAP_EXCESS_BITS(b->size)) : 8;

            for (int i = from; i < to; i++) {
                if (val == !!(*p & (1 << i))) {
                    return (8 * ((int) (p - b->bits))) + i;
                }
            }
        }

        from = 0;
        p++;
    }

    return -1;
}

// returns index of highest bit with value or -1 if there is no such bit
M_INLINE int bitmap_rfind(const bitmap_t *b, int start, bool val) {
    BITMAP_CHECK(b);

    if (start >= b->size) { return -1; }

    // TODO: optimize!!
    for (int i = start; i > 0; i--) {
        if (val == bitmap_get(b, i)) { return i; }
    }

    return -1;
}
