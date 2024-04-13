#pragma once

#include "types.h"
#include "math/linalg.h"

#define DECL_HASH_ADD(_t)                                                    \
    M_INLINE hash_t hash_add_##_t(hash_t hash, _t x) {                  \
        return                                                               \
            (hash ^                                                          \
                (((hash_t) (x)) + 0x9E3779B9u + (hash << 6) + (hash >> 2))); \
    }

DECL_HASH_ADD(u8)
DECL_HASH_ADD(u16)
DECL_HASH_ADD(u32)
DECL_HASH_ADD(u64)

DECL_HASH_ADD(i8)
DECL_HASH_ADD(i16)
DECL_HASH_ADD(i32)
DECL_HASH_ADD(i64)

DECL_HASH_ADD(usize)
DECL_HASH_ADD(isize)

DECL_HASH_ADD(char)
DECL_HASH_ADD(int)

M_INLINE hash_t hash_combine(hash_t a, hash_t b) {
    STATIC_ASSERT(sizeof(hash_t) == sizeof(u64));
    return hash_add_u64(a, b);
}

M_INLINE hash_t hash_add_str(hash_t hash, const char *s) {
    while (*s) { hash = hash_add_char(hash, *s); s++; }
    return hash;
}

M_INLINE hash_t hash_add_f32(hash_t hash, f32 x) {
    return hash_add_u32(hash, *(u32*)&x);
}

M_INLINE hash_t hash_add_f64(hash_t hash, f64 x) {
    return hash_add_u64(hash, *(u64*)&x);
}

M_INLINE hash_t hash_add_v2(hash_t hash, v2 v) {
    hash = hash_add_f32(hash, v.x);
    hash = hash_add_f32(hash, v.y);
    return hash;
}

M_INLINE hash_t hash_add_v2i(hash_t hash, v2i v) {
    hash = hash_add_i32(hash, v.x);
    hash = hash_add_i32(hash, v.y);
    return hash;
}

M_INLINE hash_t hash_add_v3(hash_t hash, v3 v) {
    hash = hash_add_f32(hash, v.x);
    hash = hash_add_f32(hash, v.y);
    hash = hash_add_f32(hash, v.z);
    return hash;
}

M_INLINE hash_t hash_add_v4(hash_t hash, v4 v) {
    hash = hash_add_f32(hash, v.x);
    hash = hash_add_f32(hash, v.y);
    hash = hash_add_f32(hash, v.z);
    hash = hash_add_f32(hash, v.w);
    return hash;
}

M_INLINE hash_t hash_add_uintptr(hash_t hash, uintptr_t x) {
    return
        (hash ^
            (((hash_t) (x)) + 0x9E3779B9u + (hash << 6) + (hash >> 2)));
}

M_INLINE hash_t hash_add_ptr(hash_t hash, void *ptr) {
    return hash_add_uintptr(hash, (uintptr_t) ptr);
}

M_INLINE hash_t hash_add_ptrs(hash_t hash, void **ptrs, int n) {
    for (int i = 0; i < n; i++) {
        hash = hash_add_ptr(hash, ptrs[i]);
    }
    return hash;
}

M_INLINE hash_t hash_add_bytes(hash_t hash, const void *ptr, int n) {
    for (int i = 0; i < n; i++) {
        hash = hash_add_u8(hash, ((u8*) ptr)[i]);
    }
    return hash;
}

