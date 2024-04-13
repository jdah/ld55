#pragma once

#include <arpa/inet.h>

#include "range.h"
#include "alloc.h"
#include "dynlist.h"
#include "math/linalg.h"

typedef struct any any_t;

typedef struct bytebuf {
    const range_t range;
    int cursor;
} bytebuf_t;

enum {
    BYTEBUF_OUT_OF_SPACE            = -1,
    BYTEBUF_NOT_ENOUGH_OUTPUT_SPACE = -2,
    BYTEBUF_NOT_ENOUGH_INPUT_SPACE  = -3,
    BYTEBUF_ANY_ERROR               = -4,
    BYTEBUF_BAD_ARRAY_SIZE          = -5,
    BYTEBUF_NOT_ENOUGH_ARRAY_DATA   = -6,
};

// wrap range in bytebuf
M_INLINE bytebuf_t bytebuf_wrap(const range_t *range) {
    return (bytebuf_t) { .range = *range, .cursor = 0 };
}

// bytes remaining in buffer
M_INLINE int bytebuf_remaining(const bytebuf_t *b) {
    return b->range.size - b->cursor;
}

// get cursor pos
M_INLINE int bytebuf_tell(const bytebuf_t *b) {
    return b->cursor;
}

// increment cursor
// returns cursor on success, else returns < 0
M_INLINE int bytebuf_skip(bytebuf_t *b, int n) {
    if (bytebuf_remaining(b) < n) { return -1; }
    b->cursor += n;
    return b->cursor;
}

// decrement cursor
// returns cursor on success, else returns < 0
M_INLINE int bytebuf_rewind(bytebuf_t *b, int n) {
    ASSERT(n >= 0);
    if (b->cursor - n < 0) { return -1; }
    b->cursor -= n;
    return b->cursor;
}

// gets what's left of the byte buffer (after cursor) as a range
M_INLINE range_t bytebuf_remaining_as_range(bytebuf_t *b) {
    return (range_t) { b->range.ptr + b->cursor, b->range.size - b->cursor };
}

// get what's been used (before cursor) as a range
M_INLINE range_t bytebuf_used_as_range(bytebuf_t *b) {
    return (range_t) { b->range.ptr, b->cursor };
}

// sets cursor
// returns cursor on success, else returns < 0
M_INLINE int bytebuf_seek(bytebuf_t *b, int cursor) {
    if (cursor < 0 || b->range.size < (usize) cursor) { return -1; }
    b->cursor = cursor;
    return b->cursor;
}

#define BYTEBUF_DECL_RW_SIZED(T, name, sz, hton, ntoh)                         \
    M_INLINE int bytebuf_peek_##name(bytebuf_t *b, T *out) {              \
        if (bytebuf_remaining(b) < (int) sz) { return BYTEBUF_OUT_OF_SPACE; }  \
        memcpy(out, &((u8*) b->range.ptr)[b->cursor], sz);                     \
        *out = ntoh(*out);                                                     \
        return sz;                                                             \
    }                                                                          \
                                                                               \
    M_INLINE int bytebuf_read_##name(bytebuf_t *b, T *out) {              \
        const int res = bytebuf_peek_##name(b, out);                           \
        if (res == sz) { b->cursor += sz; }                                    \
        return res;                                                            \
    }                                                                          \
                                                                               \
    M_INLINE int bytebuf_write_##name(bytebuf_t *b, T val) {              \
        if (bytebuf_remaining(b) < (int) sz) { return BYTEBUF_OUT_OF_SPACE; }  \
        val = hton(val);                                                       \
        memcpy(&(((u8*) b->range.ptr)[b->cursor]), &val, sz);                  \
        b->cursor += sz;                                                       \
        return sz;                                                             \
    }                                                                          \

// TODO: does nasty aliasing, this is probably undefined behavior?
#define BYTEBUF_DECL_RW_4x2(T, name)                                           \
    M_INLINE int bytebuf_peek_##name(bytebuf_t *b, T *out) {                   \
        bytebuf_peek_u32(b, (u32*) (&out->raw[0]));                            \
        bytebuf_peek_u32(b, (u32*) (&out->raw[1]));                            \
        return 8;                                                              \
    }                                                                          \
                                                                               \
    M_INLINE int bytebuf_read_##name(bytebuf_t *b, T *out) {                   \
        bytebuf_read_u32(b, (u32*) (&out->raw[0]));                            \
        bytebuf_read_u32(b, (u32*) (&out->raw[1]));                            \
        return 8;                                                              \
    }                                                                          \
                                                                               \
    M_INLINE int bytebuf_write_##name(bytebuf_t *b, T val) {                   \
        bytebuf_write_u32(b, *(u32*) (&val.raw[0]));                           \
        bytebuf_write_u32(b, *(u32*) (&val.raw[1]));                           \
        return 8;                                                              \
    }

#define IDENTITY(x) (x)
BYTEBUF_DECL_RW_SIZED(u8,   u8,   sizeof(u8),   IDENTITY, IDENTITY)
BYTEBUF_DECL_RW_SIZED(i8,   i8,   sizeof(i8),   IDENTITY, IDENTITY)
BYTEBUF_DECL_RW_SIZED(char, char, sizeof(char), IDENTITY, IDENTITY)
BYTEBUF_DECL_RW_SIZED(bool, bool, sizeof(bool), IDENTITY, IDENTITY)
#undef IDENTITY

BYTEBUF_DECL_RW_SIZED(u16,  u16,  sizeof(u16),  htons,    ntohs)
BYTEBUF_DECL_RW_SIZED(i16,  i16,  sizeof(u16),  htons,    ntohs)

BYTEBUF_DECL_RW_SIZED(u32,  u32,  sizeof(u32),  htonl,    ntohl)
BYTEBUF_DECL_RW_SIZED(i32,  i32,  sizeof(u32),  htonl,    ntohl)
BYTEBUF_DECL_RW_SIZED(f32,  f32,  sizeof(u32),  htonl,    ntohl)
BYTEBUF_DECL_RW_SIZED(uint, uint, sizeof(uint), htonl,    ntohl)
BYTEBUF_DECL_RW_SIZED(int,  int,  sizeof(int),  htonl,    ntohl)

BYTEBUF_DECL_RW_SIZED(u64,  u64,  sizeof(u64),  htonll,   ntohll)
BYTEBUF_DECL_RW_SIZED(i64,  i64,  sizeof(u64),  htonll,   ntohll)
BYTEBUF_DECL_RW_SIZED(f64,  f64,  sizeof(u64),  htonll,   ntohll)

BYTEBUF_DECL_RW_4x2(v2, v2)
BYTEBUF_DECL_RW_4x2(v2i, v2i)

#define BYTEBUF_READ_FOR_COUNT(T, name, count_var, extra_bytes) {              \
        if (count_var < 0) { return BYTEBUF_BAD_ARRAY_SIZE; }                  \
        const int sz = count_var * sizeof(T);                                  \
        if ((int) dst->size < sz) {                                            \
            return BYTEBUF_NOT_ENOUGH_INPUT_SPACE;                             \
        }                                                                      \
        if (bytebuf_remaining(b) < sz) { return BYTEBUF_OUT_OF_SPACE; }        \
        int res, total = extra_bytes;                                          \
        for (int i = 0; i < count_var; i++) {                                  \
            T val;                                                             \
            if ((res = bytebuf_read_##name(b, &val)) != sizeof(T)) {           \
                return res;                                                    \
            }                                                                  \
            total += res;                                                      \
            memcpy(dst->ptr + (i * sizeof(T)), &val, sizeof(T));               \
        }                                                                      \
        return total;                                                          \
    }                                                                          \

#define BYTEBUF_WRITE_FOR_COUNT(T, name, count_var, extra_bytes) {             \
        if (count_var < 0) { return BYTEBUF_BAD_ARRAY_SIZE; }                  \
        const int sz = count_var * sizeof(T);                                  \
        if ((int) src->size < sz) {                                            \
            LOG("not enough array data!");                                     \
            return BYTEBUF_NOT_ENOUGH_ARRAY_DATA;                              \
        }                                                                      \
        if (bytebuf_remaining(b) < sz) { return BYTEBUF_OUT_OF_SPACE; }        \
        int res, total = extra_bytes;                                          \
        for (int i = 0; i < count_var; i++) {                                  \
            T val;                                                             \
            memcpy(&val, src->ptr + (i * sizeof(T)), sizeof(T));               \
            if ((res = bytebuf_write_##name(b, val)) != sizeof(T)) {           \
                return res;                                                    \
            }                                                                  \
            total += res;                                                      \
        }                                                                      \
        return total;                                                          \
    }                                                                          \

#define BYTEBUF_DECL_RW_ARRAY(T, name)                                         \
    M_INLINE int bytebuf_read_##name##_array(                             \
        bytebuf_t *b, const range_t *dst, int count) {                         \
        BYTEBUF_READ_FOR_COUNT(T, name, count, 0)                              \
    }                                                                          \
                                                                               \
    M_INLINE int bytebuf_read_##name##_array_var(                         \
        bytebuf_t *b, const range_t *dst) {                                    \
        if (bytebuf_remaining(b) < 4) { return BYTEBUF_OUT_OF_SPACE; }         \
        int res, count;                                                        \
        if ((res = bytebuf_read_int(b, &count)) != 4) { return res; }          \
        BYTEBUF_READ_FOR_COUNT(T, name, count, 4)                              \
    }                                                                          \
                                                                               \
    M_INLINE int bytebuf_read_##name##_array_var_alloc(                   \
        bytebuf_t *b, allocator_t *allocator, range_t *out) {                  \
        int res, count;                                                        \
        if ((res = bytebuf_peek_int(b, &count)) != 4) { return res; }          \
        *out = mem_alloc_range(allocator, count * sizeof(T));                  \
        res = bytebuf_read_##name##_array_var(b, out);                         \
        if (res < 0) {                                                         \
            mem_free(allocator, out->ptr);                                     \
            *out = (range_t) { 0 };                                            \
        }                                                                      \
        return res;                                                            \
    }                                                                          \
                                                                               \
    M_INLINE int bytebuf_write_##name##_array(                            \
        bytebuf_t *b, const range_t *src, int count) {                         \
        BYTEBUF_WRITE_FOR_COUNT(T, name, count, 0)                             \
    }                                                                          \
                                                                               \
    M_INLINE int bytebuf_write_##name##_array_var(                        \
        bytebuf_t *b, const range_t *src, int count) {                         \
        if (bytebuf_remaining(b) < 4) { return BYTEBUF_OUT_OF_SPACE; }         \
        if (count < 0) { return BYTEBUF_BAD_ARRAY_SIZE; }                      \
        int res;                                                               \
        if ((res = bytebuf_write_int(b, count)) != 4) { return res; }          \
        BYTEBUF_WRITE_FOR_COUNT(T, name, count, 4)                             \
    }                                                                          \

BYTEBUF_DECL_RW_ARRAY(u8, u8)
BYTEBUF_DECL_RW_ARRAY(i8, i8)
BYTEBUF_DECL_RW_ARRAY(char, char)
BYTEBUF_DECL_RW_ARRAY(bool, bool)

BYTEBUF_DECL_RW_ARRAY(u16, u16)
BYTEBUF_DECL_RW_ARRAY(i16, i16)

BYTEBUF_DECL_RW_ARRAY(u32, u32)
BYTEBUF_DECL_RW_ARRAY(i32, i32)
BYTEBUF_DECL_RW_ARRAY(f32, f32)
BYTEBUF_DECL_RW_ARRAY(uint, uint)
BYTEBUF_DECL_RW_ARRAY(int, int)

BYTEBUF_DECL_RW_ARRAY(u64, u64)
BYTEBUF_DECL_RW_ARRAY(i64, i64)
BYTEBUF_DECL_RW_ARRAY(f64, f64)

BYTEBUF_DECL_RW_ARRAY(v2, v2)
BYTEBUF_DECL_RW_ARRAY(v2i, v2i)

// WARNING: uses thread_scratch()!
M_INLINE int bytebuf_read_str(
    bytebuf_t *b,
    allocator_t *allocator,
    char **out) {
    DYNLIST(char) str = dynlist_create(char, thread_scratch());
    char ch;
    int res, total = 0;
    while ((res = bytebuf_read_char(b, &ch)) == 1 && ch != '\0') {
        *dynlist_push(str) = ch;
        total++;
    }

    ASSERT(ch == '\0');
    total++;

    if (res != 1) { return res; }

    *dynlist_push(str) = '\0';

    *out = mem_strdup(allocator, str);

    return total;
}

// NOTE: includes NULL terminator!
M_INLINE int bytebuf_write_str(bytebuf_t *b, const char *str) {
    int res, total = 0;
    while (*str) {
        if ((res = bytebuf_write_char(b, *str)) != 1) { return res; }
        str++;
        total++;
    }

    if ((res = bytebuf_write_char(b, *str)) != 1) { return res; }
    total++;
    return total;
}

int bytebuf_read_any(bytebuf_t *b, any_t *dst);

int bytebuf_write_any(bytebuf_t *b, const any_t *src);

int bytebuf_read_any_array(
    bytebuf_t *buf, const range_t *dst, int count, allocator_t *allocator);

int bytebuf_read_any_array_var(
    bytebuf_t *buf, const range_t *dst, allocator_t *allocator);

int bytebuf_write_any_array(
    bytebuf_t *buf, const range_t *src, int count);

int bytebuf_write_any_array_var(
    bytebuf_t *buf, const range_t *src, int count);

#ifdef UTIL_IMPL

#include "any.h"

int bytebuf_read_any(bytebuf_t *b, any_t *dst) {
    const range_t range = bytebuf_remaining_as_range(b);
    const int n = any_deserialize(dst, &range);
    if (n <= 0) { return BYTEBUF_ANY_ERROR; }
    bytebuf_skip(b, n);
    return n;
}

int bytebuf_write_any(bytebuf_t *b, const any_t *src) {
    const range_t range = bytebuf_remaining_as_range(b);
    const int n = any_serialize(src, &range);
    if (n <= 0) { return BYTEBUF_ANY_ERROR; }
    bytebuf_skip(b, n);
    return n;
}

int bytebuf_read_any_array(
    bytebuf_t *buf, const range_t *dst, int count, allocator_t *allocator) {
    if (count == 0) { return 0; }

    int res, i = 0, total = 0;
    do {
        any_t *a = (any_t*) (dst->ptr + (i * sizeof(any_t)));
        any_init(a, allocator);
        res = bytebuf_read_any(buf, a);
        if (res <= 0) { return res; }
        total += res;
        i++;
    } while (i != count);

    return total;
}

int bytebuf_read_any_array_var(
    bytebuf_t *buf, const range_t *dst, allocator_t *allocator) {
    int res, count;
    if ((res = bytebuf_read_int(buf, &count)) < 0) { return res; }
    if (count < 0) { return BYTEBUF_BAD_ARRAY_SIZE; }
    return bytebuf_read_any_array(buf, dst, count, allocator);
}

int bytebuf_write_any_array(
    bytebuf_t *buf, const range_t *src, int count) {
    if (count == 0) { return 0; }

    int res, i = 0, total = 0;
    do {
        any_t *a = (any_t*) (src->ptr + (i * sizeof(any_t)));
        res = bytebuf_write_any(buf, a);
        if (res <= 0) { return res; }
        total += res;
        i++;
    } while (i != count);

    return total;
}

int bytebuf_write_any_array_var(
    bytebuf_t *buf, const range_t *src, int count) {
    if (count < 0) { return BYTEBUF_BAD_ARRAY_SIZE; }
    int res;
    if ((res = bytebuf_write_int(buf, count)) < 0) { return res; }
    return bytebuf_write_any_array(buf, src, count);
}

#endif // ifdef UTIL_IMPL
