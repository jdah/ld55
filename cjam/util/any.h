#pragma once

#include "alloc.h"
#include "range.h"
#include "hash.h"

// basic "any" type for a number of possible primitive types, supporting some
// simple type-safe get/set operations

typedef enum : u8 {
    ANY_NONE    = 0,
    ANY_INT     = 1,
    ANY_UINT    = 2,
    ANY_U8      = 3,
    ANY_U16     = 4,
    ANY_U32     = 5,
    ANY_U64     = 6,
    ANY_I8      = 7,
    ANY_I16     = 8,
    ANY_I32     = 9,
    ANY_I64     = 10,
    ANY_F32     = 11,
    ANY_F64     = 12,
    ANY_BOOL    = 13,
    ANY_VEC2    = 14,
    ANY_IVEC2   = 17,
    ANY_STR     = 20,
    ANY_CHAR    = 21,
    ANY_BYTES   = 22,
    ANY_KVSTORE = 23,
} any_type_e;

#define ANY_COUNT 22

typedef struct any {
    allocator_t *allocator;

    union {
        int int_value;
        uint uint_value;
        u8 u8_value;
        u16 u16_value;
        u32 u32_value;
        u64 u64_value;
        i8 i8_value;
        i16 i16_value;
        i32 i32_value;
        i64 i64_value;
        bool bool_value;
        f32 f32_value;
        f64 f64_value;
        v2 vec2_value;
        v2i ivec2_value;
        char *str_value;
        char char_value;
        struct { u8 *arr; int n; } bytes_value;
        kvstore_t kvstore_value;
    };

    any_type_e type;
} any_t;

#define any_of_none()    ((any_t) { .type = ANY_NONE                       })
#define any_of_int(_v)   ((any_t) { .type = ANY_INT,   .int_value   = (_v) })
#define any_of_uint(_v)  ((any_t) { .type = ANY_UINT,  .uuint_value = (_v) })
#define any_of_u8(_v)    ((any_t) { .type = ANY_U8,    .u8_value    = (_v) })
#define any_of_i8(_v)    ((any_t) { .type = ANY_I8,    .i8_value    = (_v) })
#define any_of_u16(_v)   ((any_t) { .type = ANY_U16,   .u16_value   = (_v) })
#define any_of_i16(_v)   ((any_t) { .type = ANY_I16,   .i16_value   = (_v) })
#define any_of_u32(_v)   ((any_t) { .type = ANY_U32,   .u32_value   = (_v) })
#define any_of_i32(_v)   ((any_t) { .type = ANY_I32,   .i32_value   = (_v) })
#define any_of_u64(_v)   ((any_t) { .type = ANY_U64,   .u64_value   = (_v) })
#define any_of_i64(_v)   ((any_t) { .type = ANY_I64,   .i64_value   = (_v) })
#define any_of_bool(_v)  ((any_t) { .type = ANY_BOOL,  .bool_value  = (_v) })
#define any_of_f32(_v)   ((any_t) { .type = ANY_F32,   .f32_value   = (_v) })
#define any_of_f64(_v)   ((any_t) { .type = ANY_F64,   .f64_value   = (_v) })
#define any_of_vec2(_v)  ((any_t) { .type = ANY_VEC2,  .vec2_value  = (_v) })
#define any_of_ivec2(_v) ((any_t) { .type = ANY_IVEC2, .ivec2_value = (_v) })

M_INLINE void any_init(any_t *a, allocator_t *allocator) {
    *a = (any_t) { .allocator = allocator, .type = ANY_NONE };
}

M_INLINE void any_destroy(any_t *a) {
    if (a->allocator) {
        switch (a->type) {
        case ANY_STR:
            if (a->str_value) {
                mem_free(a->allocator, a->str_value);
            }
            break;
        case ANY_BYTES:
            if (a->bytes_value.arr) {
                mem_free(a->allocator, a->bytes_value.arr);
            }
            break;
        default:
        }
    }

    *a = (any_t) { 0 };
}

M_INLINE bool any_is(const any_t *a, any_type_e type) {
    return a->type == type;
}

#define DEFINE_GET_SET(T, t_name, field_name, enum_val)                        \
    M_INLINE bool any_get_##t_name(const any_t *a, T *out) {              \
        if (a->type == enum_val) {                                             \
            *out = a->field_name;                                              \
            return true;                                                       \
        }                                                                      \
        return false;                                                          \
    }                                                                          \
                                                                               \
    M_INLINE T any_get_##t_name##_or_default(const any_t *a, T _default) {\
        return a->type == enum_val ? a->field_name : _default;                 \
    }                                                                          \
                                                                               \
    M_INLINE void any_set_##t_name(any_t *a, T value) {                   \
        allocator_t *allocator = a->allocator;                                 \
        any_destroy(a);                                                        \
        any_init(a, allocator);                                                \
        a->type = enum_val;                                                    \
        a->field_name = value;                                                 \
    }

DEFINE_GET_SET(int,    int,   int_value,   ANY_INT)
DEFINE_GET_SET(uint,   uint,  uint_value,  ANY_UINT)
DEFINE_GET_SET(u8,     u8,    u8_value,    ANY_U8)
DEFINE_GET_SET(u16,    u16,   u16_value,   ANY_U16)
DEFINE_GET_SET(u32,    u32,   u32_value,   ANY_U32)
DEFINE_GET_SET(u64,    u64,   u64_value,   ANY_U64)
DEFINE_GET_SET(i8,     i8,    i8_value,    ANY_I8)
DEFINE_GET_SET(i16,    i16,   i16_value,   ANY_I16)
DEFINE_GET_SET(i32,    i32,   i32_value,   ANY_I32)
DEFINE_GET_SET(i64,    i64,   i64_value,   ANY_I64)
DEFINE_GET_SET(f32,    f32,   f32_value,   ANY_F32)
DEFINE_GET_SET(f64,    f64,   f64_value,   ANY_F64)
DEFINE_GET_SET(bool,   bool,  bool_value,  ANY_BOOL)
DEFINE_GET_SET(vec2s,  vec2,  vec2_value,  ANY_VEC2)
DEFINE_GET_SET(ivec2s, ivec2, ivec2_value, ANY_IVEC2)
DEFINE_GET_SET(char,   char,  char_value,  ANY_CHAR)

#undef DEFINE_GET_SET

M_INLINE const char *any_get_str(const any_t *a) {
    return a->type == ANY_STR ? a->str_value : NULL;
}

M_INLINE const char *any_get_str_or_default(
    const any_t *a,
    const char *_default) {
    const char *res = any_get_str(a);
    return res ? res : _default;
}

M_INLINE void any_set_str(any_t *a, const char *value) {
    ASSERT(a->allocator);
    allocator_t *allocator = a->allocator;
    any_destroy(a);
    any_init(a, allocator);
    a->type = ANY_STR;
    a->str_value = mem_alloc_inplace(a->allocator, strlen(value)  + 1, value);
}

M_INLINE void any_set_bytes(any_t *a, const range_t *data) {
    ASSERT(a->allocator);
    allocator_t *allocator = a->allocator;
    any_destroy(a);
    any_init(a, allocator);

    a->type = ANY_BYTES;
    a->bytes_value.n = data->size;
    a->bytes_value.arr = mem_alloc_inplace(allocator, data->size, data->ptr);
}

M_INLINE range_t any_get_bytes(const any_t *a) {
    ASSERT(a->type == ANY_BYTES);
    return (range_t) { .ptr = a->bytes_value.arr, .size = a->bytes_value.n };
}

M_INLINE const kvstore_t *any_get_kvstore(const any_t *a) {
    return a->type == ANY_KVSTORE ? &a->kvstore_value : NULL;
}

void any_set_kvstore(any_t *a, const kvstore_t *kvs);

// gets ANY numeric value as an f64
M_INLINE bool any_get_as_f64(const any_t *a, f64 *out) {
    switch (a->type) {
    case ANY_INT:  *out = a->int_value;  return true;
    case ANY_UINT: *out = a->uint_value; return true;
    case ANY_U8:   *out = a->u8_value;   return true;
    case ANY_U16:  *out = a->u16_value;  return true;
    case ANY_U32:  *out = a->u32_value;  return true;
    case ANY_U64:  *out = a->u64_value;  return true;
    case ANY_I8:   *out = a->i8_value;   return true;
    case ANY_I16:  *out = a->i16_value;  return true;
    case ANY_I32:  *out = a->i32_value;  return true;
    case ANY_I64:  *out = a->i64_value;  return true;
    case ANY_F32:  *out = a->f32_value;  return true;
    case ANY_F64:  *out = a->f64_value;  return true;
    case ANY_BOOL: *out = a->bool_value; return true;
    case ANY_CHAR: *out = a->char_value; return true;
    case ANY_NONE: *out = 0;             return true;
    default: return false;
    }
}

// gets ANY numeric value as an f64 or default
M_INLINE f64 any_get_as_f64_or_default(const any_t *a, f64 _default) {
    f64 value;
    return any_get_as_f64(a, &value) ? value : _default;
}

// gets ANY numeric value as an f64 or default
M_INLINE f64 any_get_as_f64_or_die(const any_t *a) {
    f64 value;
    ASSERT(any_get_as_f64(a, &value));
    return value;
}

// true if same type and equal values
bool any_eq(const any_t *a, const any_t *b);

void any_copy(any_t *dst, const any_t *src);

void any_set_fmt(any_t *a, const char *fmt, ...);

void any_set_vfmt(any_t *a, const char *fmt, va_list args);

// for map
typedef struct map map_t;
M_INLINE void map_any_free(map_t*, void *p) { any_destroy(p); }

// gets size required to serialize any
int any_serialize_size(const any_t *any);

// serializes an any, returns number of bytes read, <= 0 on error
int any_serialize(const any_t *a, const range_t *data);

// deserializes an any, returns number of bytes written, <= 0 on error
int any_deserialize(any_t *a, const range_t *data);

// tries to massage any into target type
// returns false on failure
bool any_try_massage_type(any_t *a, any_type_e type);

// any -> string
char *any_to_str(const any_t *a, allocator_t *al);

// compute hash for any
hash_t any_hash(const any_t *a);

#ifdef UTIL_IMPL

#include "hash.h"
#include "bytebuf.h"
#include "kvstore.h"

void any_set_fmt(any_t *a, const char *fmt, ...) {
    va_list args;
    va_start(args, fmt);
    any_set_vfmt(a, fmt, args);
    va_end(args);
}

void any_set_vfmt(any_t *a, const char *fmt, va_list args) {
    char *dst;
    vasprintf(&dst, fmt, args);
    any_set_str(a, dst);
    free(dst);
}

int any_serialize_size(const any_t *any) {
    switch (any->type) {
    case ANY_NONE:  return 1;
    case ANY_INT:   return 1 + sizeof(any->int_value);
    case ANY_UINT:  return 1 + sizeof(any->uint_value);
    case ANY_U8:    return 1 + sizeof(any->u8_value);
    case ANY_U16:   return 1 + sizeof(any->u16_value);
    case ANY_U32:   return 1 + sizeof(any->u32_value);
    case ANY_U64:   return 1 + sizeof(any->u64_value);
    case ANY_I8:    return 1 + sizeof(any->i8_value);
    case ANY_I16:   return 1 + sizeof(any->i16_value);
    case ANY_I32:   return 1 + sizeof(any->i32_value);
    case ANY_I64:   return 1 + sizeof(any->i64_value);
    case ANY_F32:   return 1 + sizeof(any->f32_value);
    case ANY_F64:   return 1 + sizeof(any->f64_value);
    case ANY_BOOL:  return 1 + sizeof(any->bool_value);
    case ANY_VEC2:  return 1 + sizeof(any->vec2_value);
    case ANY_IVEC2: return 1 + sizeof(any->ivec2_value);
    case ANY_CHAR:  return 1 + sizeof(any->char_value);
    case ANY_STR:   return 1 + strlen(any->str_value) + 1;
    case ANY_BYTES: return 1 + sizeof(int) + any->bytes_value.n;
    case ANY_KVSTORE: return 1 + kvstore_serialize_size(&any->kvstore_value);
    }
}

static const int offsets[ANY_COUNT] = {
    [ANY_INT]   = offsetof(any_t, int_value),
    [ANY_UINT]  = offsetof(any_t, uint_value),
    [ANY_U8]    = offsetof(any_t, u8_value),
    [ANY_U16]   = offsetof(any_t, u16_value),
    [ANY_U32]   = offsetof(any_t, u32_value),
    [ANY_U64]   = offsetof(any_t, u64_value),
    [ANY_I8]    = offsetof(any_t, i8_value),
    [ANY_I16]   = offsetof(any_t, i16_value),
    [ANY_I32]   = offsetof(any_t, i32_value),
    [ANY_I64]   = offsetof(any_t, i64_value),
    [ANY_F32]   = offsetof(any_t, f32_value),
    [ANY_F64]   = offsetof(any_t, f64_value),
    [ANY_BOOL]  = offsetof(any_t, bool_value),
    [ANY_VEC2]  = offsetof(any_t, vec2_value),
    [ANY_IVEC2] = offsetof(any_t, ivec2_value),
    [ANY_CHAR]  = offsetof(any_t, char_value),
};

static const int sizes[ANY_COUNT] = {
    [ANY_INT]   = sizeof_field(any_t, int_value),
    [ANY_UINT]  = sizeof_field(any_t, uint_value),
    [ANY_U8]    = sizeof_field(any_t, u8_value),
    [ANY_U16]   = sizeof_field(any_t, u16_value),
    [ANY_U32]   = sizeof_field(any_t, u32_value),
    [ANY_U64]   = sizeof_field(any_t, u64_value),
    [ANY_I8]    = sizeof_field(any_t, i8_value),
    [ANY_I16]   = sizeof_field(any_t, i16_value),
    [ANY_I32]   = sizeof_field(any_t, i32_value),
    [ANY_I64]   = sizeof_field(any_t, i64_value),
    [ANY_F32]   = sizeof_field(any_t, f32_value),
    [ANY_F64]   = sizeof_field(any_t, f64_value),
    [ANY_BOOL]  = sizeof_field(any_t, bool_value),
    [ANY_VEC2]  = sizeof_field(any_t, vec2_value),
    [ANY_IVEC2] = sizeof_field(any_t, ivec2_value),
    [ANY_CHAR]  = sizeof_field(any_t, char_value),
};

int any_serialize(const any_t *a, const range_t *data) {
    const int expected_size = any_serialize_size(a);
    ASSERT((int) data->size >= expected_size);

    int res;
    bytebuf_t buf = bytebuf_wrap(data);

    ASSERT(bytebuf_write_u8(&buf, a->type) > 0);

    switch (a->type) {
    case ANY_KVSTORE:;
        const int sz = kvstore_serialize_size(&a->kvstore_value);
        const range_t range = bytebuf_remaining_as_range(&buf);
        ASSERT(kvstore_serialize(&a->kvstore_value, &range) == sz);
        bytebuf_skip(&buf, sz);
        break;
    case ANY_BYTES:
        ASSERT(
            bytebuf_write_u8_array_var(
                &buf,
                &(range_t) { a->bytes_value.arr, a->bytes_value.n },
                a->bytes_value.n) > 0);
        break;
    case ANY_STR:
        ASSERT((res = bytebuf_write_str(&buf, a->str_value) > 0), "%d", res);
        break;
    case ANY_U16:
    case ANY_I16:
        ASSERT(
            bytebuf_write_u16(
                &buf, *(u16*) (((u8*) a) + offsets[a->type])) > 0);
        break;
    case ANY_F32:
    case ANY_INT:
    case ANY_UINT:
    case ANY_U32:
    case ANY_I32:
        ASSERT(
            bytebuf_write_u32(
                &buf, *(u32*) (((u8*) a) + offsets[a->type])) > 0);
        break;
    case ANY_F64:
    case ANY_U64:
    case ANY_I64:
        ASSERT(
            bytebuf_write_u64(
                &buf, *(u64*) (((u8*) a) + offsets[a->type])) > 0);
        break;
    case ANY_VEC2:
    case ANY_IVEC2:
        ASSERT(
            bytebuf_write_v2i(
                &buf, *(ivec2s*) (((u8*) a) + offsets[a->type])) > 0);
        break;
    case ANY_U8:
    case ANY_I8:
    case ANY_BOOL:
    case ANY_CHAR:
        ASSERT(
            bytebuf_write_u8(
                &buf, *(u8*) (((u8*) a) + offsets[a->type])) > 0);
        break;
    case ANY_NONE:
    }

    ASSERT(bytebuf_tell(&buf) == expected_size);
    return expected_size;
}

int any_deserialize(any_t *a, const range_t *data) {
    if (data->size < 1) { goto fail; }
    allocator_t *allocator = a->allocator;
    any_destroy(a);
    any_init(a, allocator);

    bytebuf_t buf = bytebuf_wrap(data);

    u8 type;
    ASSERT(bytebuf_read_u8(&buf, &type) > 0);
    a->type = type;

    switch (a->type) {
    case ANY_KVSTORE: {
        ASSERT(a->allocator);
        kvstore_init(&a->kvstore_value, a->allocator);
        const range_t range = bytebuf_remaining_as_range(&buf);
        ASSERT(kvstore_deserialize(&a->kvstore_value, &range) > 0);
    } break;
    case ANY_BYTES: {
        ASSERT(a->allocator);
        range_t range;
        const int res =
            bytebuf_read_u8_array_var_alloc(&buf, a->allocator, &range);
        ASSERT(res > 0, "%d", res);
        a->bytes_value.arr = range.ptr;
        a->bytes_value.n = range.size;
    } break;
    case ANY_STR:
        ASSERT(a->allocator);
        ASSERT(bytebuf_read_str(&buf, a->allocator, &a->str_value) > 0);
        break;
    case ANY_U16:
    case ANY_I16:
        ASSERT(
            bytebuf_read_u16(&buf, (u16*) (((u8*) a) + offsets[a->type])) > 0);
        break;
    case ANY_F32:
    case ANY_UINT:
    case ANY_INT:
    case ANY_U32:
    case ANY_I32:
        ASSERT(
            bytebuf_read_u32(&buf, (u32*) (((u8*) a) + offsets[a->type])) > 0);
        break;
    case ANY_F64:
    case ANY_U64:
    case ANY_I64:
        ASSERT(
            bytebuf_read_u64(&buf, (u64*) (((u8*) a) + offsets[a->type])) > 0);
        break;
    case ANY_VEC2:
    case ANY_IVEC2:
        ASSERT(
            bytebuf_read_v2i(
                &buf, (ivec2s*) (((u8*) a) + offsets[a->type])) > 0);
        break;
    case ANY_BOOL:
    case ANY_I8:
    case ANY_U8:
    case ANY_CHAR:
        ASSERT(
            bytebuf_read_u8(
                &buf, (u8*) (((u8*) a) + offsets[a->type])) > 0);
        break;
    case ANY_NONE: break;
    }

    return any_serialize_size(a);
fail:
    return -1;
}

bool any_try_massage_type(any_t *a, any_type_e type) {
    // TODO: actually do this properly, this is hella bad right now for
    // conversions that need to maintain accuracy

    switch (a->type) {
    case ANY_BYTES:
    case ANY_STR:
    case ANY_VEC2:
    case ANY_IVEC2:
    case ANY_NONE:
        return false;
    default:
    }

    f64 val;
    ASSERT(any_get_as_f64(a, &val));

    switch (type) {
    case ANY_INT:  any_set_int(a,  (int)  val); break;
    case ANY_UINT: any_set_uint(a, (uint) val); break;
    case ANY_U8:   any_set_u8(a,   (u8)   val); break;
    case ANY_U16:  any_set_u16(a,  (u16)  val); break;
    case ANY_U32:  any_set_u32(a,  (u32)  val); break;
    case ANY_U64:  any_set_u64(a,  (u64)  val); break;
    case ANY_I8:   any_set_i8(a,   (i8)   val); break;
    case ANY_I16:  any_set_i16(a,  (i16)  val); break;
    case ANY_I32:  any_set_i32(a,  (i32)  val); break;
    case ANY_I64:  any_set_i64(a,  (i64)  val); break;
    case ANY_F32:  any_set_f32(a,  (f32)  val); break;
    case ANY_F64:  any_set_f64(a,  (f64)  val); break;
    case ANY_BOOL: any_set_bool(a, (bool) val); break;
    case ANY_CHAR: any_set_char(a, (char) val); break;
    default: ASSERT(false);
    }

    return true;
}

void any_set_kvstore(any_t *a, const kvstore_t *kvs) {
    ASSERT(a->allocator);
    allocator_t *allocator = a->allocator;
    any_destroy(a);
    any_init(a, allocator);
    a->type = ANY_KVSTORE;
    kvstore_init(&a->kvstore_value, allocator);
    kvstore_copy(&a->kvstore_value, kvs);
}

bool any_eq(const any_t *a, const any_t *b) {
    if (a->type != b->type) { return false; }

    switch (a->type) {
    case ANY_NONE:  return true;
    case ANY_INT: return a->int_value == b->int_value;
    case ANY_UINT: return a->uint_value == b->uint_value;
    case ANY_U8: return a->u8_value == b->u8_value;
    case ANY_U16: return a->u16_value == b->u16_value;
    case ANY_U32: return a->u32_value == b->u32_value;
    case ANY_U64: return a->u64_value == b->u64_value;
    case ANY_I8: return a->i8_value == b->i8_value;
    case ANY_I16: return a->i16_value == b->i16_value;
    case ANY_I32: return a->i32_value == b->i32_value;
    case ANY_I64: return a->i64_value == b->i64_value;
    case ANY_F32: return a->f32_value == b->f32_value;
    case ANY_F64: return a->f64_value == b->f64_value;
    case ANY_BOOL: return a->bool_value == b->bool_value;
    case ANY_VEC2: return v2_eqv(a->vec2_value, b->vec2_value);
    case ANY_IVEC2: return v2i_eqv(a->ivec2_value, b->ivec2_value);
    case ANY_CHAR:  return a->char_value == b->char_value;
    case ANY_STR:   return !strcmp(a->str_value, b->str_value);
    case ANY_BYTES:
        return a->bytes_value.n == b->bytes_value.n
            && !memcmp(
                a->bytes_value.arr,
                b->bytes_value.arr,
                a->bytes_value.n);
    case ANY_KVSTORE:
        return kvstore_eq(&a->kvstore_value, &b->kvstore_value);
    }
}



void any_copy(any_t *dst, const any_t *src) {
    allocator_t *allocator = dst->allocator;
    any_destroy(dst);
    dst->allocator = allocator;
    dst->type = src->type;

    switch (src->type) {
    case ANY_KVSTORE:
        ASSERT(allocator);
        kvstore_init(&dst->kvstore_value, allocator);
        kvstore_copy(&dst->kvstore_value, &src->kvstore_value);
        break;
    case ANY_BYTES: {
        ASSERT(allocator);
        dst->bytes_value.n = src->bytes_value.n;
        dst->bytes_value.arr =
            mem_alloc_inplace(
                dst->allocator,
                src->bytes_value.n,
                src->bytes_value.arr);
    } break;
    case ANY_STR: {
        ASSERT(allocator);
        dst->str_value =
            mem_alloc_inplace(
                dst->allocator,
                strlen(src->str_value) + 1,
                src->str_value);
    } break;
    case ANY_INT:
    case ANY_UINT:
    case ANY_U8:
    case ANY_U16:
    case ANY_U32:
    case ANY_U64:
    case ANY_I8:
    case ANY_I16:
    case ANY_I32:
    case ANY_I64:
    case ANY_F32:
    case ANY_F64:
    case ANY_BOOL:
    case ANY_VEC2:
    case ANY_IVEC2:
    case ANY_CHAR: {
        const u8 *psrc = ((u8*) src) + offsets[src->type];
        u8 *pdst = ((u8*) dst) + offsets[dst->type];
        memcpy(pdst, psrc, sizes[dst->type]);
    } break;
    case ANY_NONE:
    }
}

hash_t any_hash(const any_t *any) {
    hash_t h = 0x12345;

    switch (any->type) {
    case ANY_INT:   h = hash_add_int(h,   any->int_value);   break;
    case ANY_UINT:  h = hash_add_u32(h,   any->uint_value);  break;
    case ANY_U8:    h = hash_add_u8(h,    any->u8_value);    break;
    case ANY_U16:   h = hash_add_u16(h,   any->u16_value);   break;
    case ANY_U32:   h = hash_add_u32(h,   any->u32_value);   break;
    case ANY_U64:   h = hash_add_u64(h,   any->u64_value);   break;
    case ANY_I8:    h = hash_add_i8(h,    any->i8_value);    break;
    case ANY_I16:   h = hash_add_i16(h,   any->i16_value);   break;
    case ANY_I32:   h = hash_add_i32(h,   any->i32_value);   break;
    case ANY_I64:   h = hash_add_i64(h,   any->i64_value);   break;
    case ANY_F32:   h = hash_add_f32(h,   any->f32_value);   break;
    case ANY_F64:   h = hash_add_f64(h,   any->f64_value);   break;
    case ANY_VEC2:  h = hash_add_v2(h,  any->vec2_value);  break;
    case ANY_IVEC2: h = hash_add_v2i(h, any->ivec2_value); break;
    case ANY_CHAR:  h = hash_add_char(h,  any->char_value);  break;
    case ANY_STR:   h = hash_add_str(h,   any->str_value);   break;
    case ANY_BOOL:  h = hash_add_u8(h,    any->bool_value);  break;
    case ANY_BYTES:
        h = hash_add_bytes(h, any->bytes_value.arr, any->bytes_value.n);
        break;
    case ANY_KVSTORE:
        h = hash_combine(h, kvstore_hash(&any->kvstore_value));
        break;
    case ANY_NONE:
    }

    return h;
}

char *any_to_str(const any_t *a, allocator_t *al) {
    switch (a->type) {
    case ANY_INT:   return mem_strfmt(al, "%"   PRIi32, a->int_value);
    case ANY_UINT:  return mem_strfmt(al, "%"   PRIu32, a->uint_value);
    case ANY_U8:    return mem_strfmt(al, "%"   PRIu8,  a->u8_value);
    case ANY_U16:   return mem_strfmt(al, "%"   PRIu16, a->u16_value);
    case ANY_U32:   return mem_strfmt(al, "%"   PRIu32, a->u32_value);
    case ANY_U64:   return mem_strfmt(al, "%"   PRIu64, a->u64_value);
    case ANY_I8:    return mem_strfmt(al, "%"   PRIi8,  a->i8_value);
    case ANY_I16:   return mem_strfmt(al, "%"   PRIi16, a->i16_value);
    case ANY_I32:   return mem_strfmt(al, "%"   PRIi32, a->i32_value);
    case ANY_I64:   return mem_strfmt(al, "%"   PRIi64, a->i64_value);
    case ANY_F32:   return mem_strfmt(al, "%"   PRIf32, a->f32_value);
    case ANY_F64:   return mem_strfmt(al, "%"   PRIf64, a->f64_value);
    case ANY_VEC2:  return mem_strfmt(al, "%"   PRIv2,  FMTv2(a->vec2_value));
    case ANY_IVEC2: return mem_strfmt(al, "%"   PRIv2i, FMTv2i(a->ivec2_value));
    case ANY_CHAR:  return mem_strfmt(al, "%c", a->char_value);
    case ANY_STR:   return mem_strfmt(al, "%s", a->str_value);
    case ANY_BOOL:
        return mem_strfmt(al, "%s", a->bool_value ? "true" : "false");
    case ANY_BYTES:;
        DYNLIST(char) chars =
            dynlist_create(
                char,
                thread_scratch(),
                2 + (a->bytes_value.n * 5) + 1);

        *dynlist_push(chars) = '[';
        for (int i = 0; i < a->bytes_value.n; i++) {
            char hex[8];
            const int res =
                snprintf(hex, sizeof(hex), "0x%02X", a->bytes_value.arr[i]);
            ASSERT(res == 4);

            const int offset = dynlist_size(chars);
            dynlist_resize_no_contract(chars, offset + 4);
            memcpy(&chars[offset], hex, 4);

            if (i != a->bytes_value.n - 1) { *dynlist_push(chars) = ','; }
        }
        *dynlist_push(chars) = ']';
        *dynlist_push(chars) = '\0';
        return mem_strdup(al, chars);
    case ANY_KVSTORE:
        return kvstore_to_str(&a->kvstore_value, al);
    case ANY_NONE: return mem_strdup(al, "none");
    }
}

#endif // ifdef UTIL_IMPL
