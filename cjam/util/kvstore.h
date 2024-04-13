#pragma once

#include "types.h"

typedef struct range range_t;

// maximum size for kvstore name
#define MAX_KVSTORE_NAME_SIZE 16384

void kvstore_init(kvstore_t *kvs, allocator_t *allocator);

void kvstore_destroy(kvstore_t *kvs);

// clears kvstore
void kvstore_clear(kvstore_t *kvs);

// returns true if kvstore stores nothing
bool kvstore_empty(const kvstore_t *kvs);

// copies src to dst, both must be initialized (!)
void kvstore_copy(kvstore_t *dst, const kvstore_t *src);

// compute hash for kvstore
hash_t kvstore_hash(const kvstore_t *kvs);

// check if kvstores are equal
bool kvstore_eq(const kvstore_t *a, const kvstore_t *b);

// kvstore -> pretty string
char *kvstore_to_str(const kvstore_t *kvs, allocator_t *al);

// size of serialized kvstore
int kvstore_serialize_size(const kvstore_t *kvs);

// serlialize to list of bytes
// returns number of bytes written or <0 on error
int kvstore_serialize(const kvstore_t *kvs, const range_t *data);

// deserialize from bytes
// returns number of bytes read or <0 on error
int kvstore_deserialize(kvstore_t *kvs, const range_t *data);

const any_t *kvstore_get(const kvstore_t *kvs, const char *name);

void kvstore_set(kvstore_t *kvs, const char *name, const any_t *value);

bool kvstore_has(const kvstore_t *kvs, const char *name);

bool kvstore_remove(kvstore_t *kvs, const char *name);

void kvstore_set_fmt(kvstore_t *kvs, const char *name, const char *fmt, ...);

void kvstore_set_str(kvstore_t *kvs, const char *name, const char *value);

bool kvstore_get_str(
    const kvstore_t *kvs, const char *name, allocator_t *a, char **value);

bool kvstore_get_strn(
    const kvstore_t *kvs, const char *name, char *value, size_t sz);

void kvstore_set_bytes(
    kvstore_t *kvs,
    const char *name,
    const range_t *data);

range_t kvstore_get_bytes(
    const kvstore_t *kvs,
    const char *name);

bool kvstore_get_bytes_to(
    const kvstore_t *kvs,
    const char *name,
    const range_t *dst);

#define DECL_PRIMITIVE_GET_SET(T, t_name)                                      \
    bool kvstore_get_##t_name(                                                 \
        const kvstore_t *kvstore,                                              \
        const char *name,                                                      \
        T *out);                                                               \
                                                                               \
    T kvstore_get_##t_name##_or_default(                                       \
        const kvstore_t *kvstore,                                              \
        const char *name,                                                      \
        T _default);                                                           \
                                                                               \
    void kvstore_set_##t_name(                                                 \
        kvstore_t *kvstore,                                                    \
        const char *name,                                                      \
        T value);

DECL_PRIMITIVE_GET_SET(int, int)
DECL_PRIMITIVE_GET_SET(char, char)
DECL_PRIMITIVE_GET_SET(bool, bool)
DECL_PRIMITIVE_GET_SET(u8, u8)
DECL_PRIMITIVE_GET_SET(u16, u16)
DECL_PRIMITIVE_GET_SET(u32, u32)
DECL_PRIMITIVE_GET_SET(u64, u64)
DECL_PRIMITIVE_GET_SET(i8, i8)
DECL_PRIMITIVE_GET_SET(i16, i16)
DECL_PRIMITIVE_GET_SET(i32, i32)
DECL_PRIMITIVE_GET_SET(i64, i64)
DECL_PRIMITIVE_GET_SET(f32, f32)
DECL_PRIMITIVE_GET_SET(f64, f64)
DECL_PRIMITIVE_GET_SET(vec2s, vec2)
DECL_PRIMITIVE_GET_SET(ivec2s, ivec2)

#undef DECL_PRIMITIVE_GET_SET

#ifdef UTIL_IMPL

#include "bytebuf.h"
#include "any.h"
#include "map.h"
#include "range.h"
#include "str.h"

void kvstore_init(kvstore_t *kvs, allocator_t *allocator) {
    *kvs = (kvstore_t) { .allocator = allocator };
}

static void kvstore_ensure_map(kvstore_t *kvs) {
    if (kvs->map) { return; }

    kvs->map = mem_alloc(kvs->allocator, sizeof(*kvs->map));
    map_init(
        kvs->map,
        kvs->allocator,
        sizeof(char*),
        sizeof(any_t),
        map_hash_str,
        map_cmp_str,
        map_allocator_free,
        map_any_free,
        NULL);
}

void kvstore_destroy(kvstore_t *kvs) {
    if (!kvs->map) { return; }
    map_destroy(kvs->map);
    mem_free(kvs->allocator, kvs->map);
    *kvs = (kvstore_t) { 0 };
}

void kvstore_clear(kvstore_t *kvs) {
    if (!kvs->map) { return; }
    map_destroy(kvs->map);
}

bool kvstore_empty(const kvstore_t *kvs) {
    return !kvs->map || map_empty(kvs->map);
}

void kvstore_copy(kvstore_t *dst, const kvstore_t *src) {
    ASSERT(dst->allocator);
    ASSERT(src->allocator);

    if (!src->map || map_empty(src->map)) {
        // nothing to copy
        return;
    }

    ASSERT(map_valid(src->map));
    kvstore_ensure_map(dst);

    map_each(char*, any_t, src->map, it) {
        any_t copy;
        any_init(&copy, dst->allocator);
        any_copy(&copy, it.value);

        char *key =
            mem_alloc_inplace(
                dst->allocator,
                strlen(*it.key) + 1,
                *it.key);
        map_insert(dst->map, &key, &copy);
    }
}

hash_t kvstore_hash(const kvstore_t *kvs) {
    hash_t h = 0x18273645;
    if (!kvs->map) { return h; }
    map_each(char*, any_t, kvs->map, it) {
        h = hash_add_str(h, *it.key);
        h = hash_combine(h, any_hash(it.value));
    }
    return h;
}

bool kvstore_eq(const kvstore_t *a, const kvstore_t *b) {
    if (!a->map) { return !b->map; }

    map_each(char*, any_t, a->map, it) {
        const any_t
            *v_a = it.value,
            *v_b = kvstore_get(b, *it.key);

        if (!v_b || !any_eq(v_a, v_b)) { return false; }
    }

    return true;
}

char *kvstore_to_str(const kvstore_t *kvs, allocator_t *al) {
    if (!kvs->map) { return mem_strdup(al, "{}"); }

    strbuf_t sb = strbuf_create(thread_scratch());
    strbuf_ap_str(&sb, "{");

    bool first = true;
    map_each(char*, any_t, kvs->map, it) {
        const char *prefix = ", ";

        if (first) {
            prefix = " ";
            first = false;
        }

        strbuf_ap_fmt(
            &sb,
            "%s%s: %s",
            prefix,
            *it.key,
            any_to_str(it.value, thread_scratch()));
    }
    strbuf_ap_str(&sb, " }");
    return strbuf_dump(&sb, al);
}

// kvstore binary format:
// * KVSTORE_MAGIC
// * size (i32/4 bytes)
// * for each entry:
//   * name (null-terminated string)
//   * serialized any
// * KVSTORE_MAGIC

int kvstore_serialize_size(const kvstore_t *kvs) {
    // min size is magic + size + magic
    int size = 3 * 4;

    if (!kvs->map) {
        goto done;
    }

    ASSERT(map_valid(kvs->map));

    map_each(char*, any_t, kvs->map, it) {
        size += strlen(*it.key) + 1;
        size += any_serialize_size(it.value);
    }

done:
    return size;
}

#define KVSTORE_MAGIC 0x4A4A5B5B

int kvstore_serialize(const kvstore_t *kvs, const range_t *data) {
    const int expected_size = kvstore_serialize_size(kvs);
    bytebuf_t buf = bytebuf_wrap(data);

#define WRITE_INT(_i) do {                                                     \
        if (bytebuf_write_int(&buf, (_i)) != 4) {                              \
            WARN("out of space in bytebuf");                                   \
            dumptrace(stderr);                                                 \
            return -1;                                                         \
        }                                                                      \
    } while (0)

    WRITE_INT(KVSTORE_MAGIC);

    if (!kvs->map || map_size(kvs->map) == 0) {
        WRITE_INT(0);
        WRITE_INT(KVSTORE_MAGIC);
        ASSERT(bytebuf_tell(&buf) == 3 * 4);
        return 3 * 4;
    }

    ASSERT(map_valid(kvs->map));
    WRITE_INT(map_size(kvs->map));

    map_each(char*, any_t, kvs->map, it) {
        char *name = *it.key;
        ASSERT(strlen(name) <= MAX_KVSTORE_NAME_SIZE);
        bytebuf_write_str(&buf, name);

        // reserve bytes corresponding to size of any and write out
        const int any_size = any_serialize_size(it.value);
        if (bytebuf_remaining(&buf) < any_size) {
            WARN(
                "needed %d bytes but have %d left",
                any_size,
                bytebuf_remaining(&buf));
            return -2;
        }
        const range_t range = bytebuf_remaining_as_range(&buf);
        const int size_written = any_serialize(it.value, &range);
        ASSERT(size_written == any_size);
        bytebuf_skip(&buf, size_written);
    }

    WRITE_INT(KVSTORE_MAGIC);

#undef WRITE_INT

    ASSERT(
        bytebuf_tell(&buf) == expected_size,
        "%d / %d",
        bytebuf_tell(&buf), expected_size);
    return bytebuf_tell(&buf);
}

int kvstore_deserialize(kvstore_t *kvs, const range_t *data) {
    ASSERT(kvs->allocator);

    int res = 0;

    bytebuf_t buf = bytebuf_wrap(data);

#define READ_INT() ({                                               \
        int x;                                                      \
        if (bytebuf_read_int(&buf, &x) != 4) { res = 1; goto done; }\
        x;                                                          \
    })

    // check start magic
    if (READ_INT() != KVSTORE_MAGIC) { res = -1; goto done; }

    const int size = READ_INT();

    if (size == 0) {
        // quick exit, no need to allocate map for kvstore
        if (READ_INT() != KVSTORE_MAGIC) { res = -5; goto done; }
        goto done;
    }

    kvstore_ensure_map(kvs);

    for (int i = 0; i < size; i++) {
        char *name;
        if (bytebuf_read_str(&buf, thread_scratch(), &name) < 0) {
            res = -2;
            goto done;
        }

        // read any into tmp
        any_t any;
        any_init(&any, thread_scratch());

        const range_t range = bytebuf_remaining_as_range(&buf);
        const int n_read = any_deserialize(&any, &range);
        kvstore_set(kvs, name, &any);

        if (n_read <= 0) { res = -3; goto done; }
        bytebuf_skip(&buf, n_read);
    }

    // check end magic
    if (READ_INT() != KVSTORE_MAGIC) { res = -4; goto done; }
#undef READ_INT

done:
    // OK
    return res == 0 ? bytebuf_tell(&buf) : res;
}

const any_t *kvstore_get(const kvstore_t *kvs, const char *name) {
    return kvs->map ? map_get(any_t, kvs->map, &name) : NULL;
}

void kvstore_set(kvstore_t *kvs, const char *name, const any_t *value) {
    kvstore_ensure_map(kvs);

    any_t any;
    any_init(&any, kvs->allocator);
    any_copy(&any, value);

    char *key = mem_alloc_inplace(kvs->allocator, strlen(name) + 1, name);
    map_insert(kvs->map, &key, &any);
}

bool kvstore_has(const kvstore_t *kvs, const char *name) {
    return kvs->map ? map_contains(kvs->map, &name) : false;
}

bool kvstore_remove(kvstore_t *kvs, const char *name) {
    return kvs->map ? map_try_remove(kvs->map, &name) : false;
}

void kvstore_set_fmt(kvstore_t *kvs, const char *name, const char *fmt, ...) {
    va_list args;
    va_start(args, fmt);

    any_t any;
    any_init(&any, thread_scratch());
    any_set_str(&any, mem_vstrfmt(thread_scratch(), fmt, args));
    kvstore_set(kvs, name, &any);
}

void kvstore_set_str(kvstore_t *kvs, const char *name, const char *value) {
    any_t any;
    any_init(&any, thread_scratch());
    any_set_str(&any, value);
    kvstore_set(kvs, name, &any);
}

bool kvstore_get_str(
    const kvstore_t *kvs, const char *name, allocator_t *a, char **value) {
    const any_t *any = kvstore_get(kvs, name);
    if (!any) { return false; }
    *value = mem_strdup(a, any_get_str(any));
    return true;
}

bool kvstore_get_strn(
    const kvstore_t *kvs, const char *name, char *value, size_t sz) {
    char *str;
    if (!kvstore_get_str(kvs, name, thread_scratch(), &str)) { return false; }
    const int res = snprintf(value, sizeof(value), "%s", str);
    return res >= 0 && res < (int) sz;
}

void kvstore_set_bytes(
    kvstore_t *kvs,
    const char *name,
    const range_t *data) {
    any_t any;
    any_init(&any, thread_scratch());
    any_set_bytes(&any, data);
    kvstore_set(kvs, name, &any);
}

range_t kvstore_get_bytes(
    const kvstore_t *kvs,
    const char *name) {
    const any_t *any = kvstore_get(kvs, name);
    return any ? any_get_bytes(any) : (range_t) { 0 };
}

bool kvstore_get_bytes_to(
    const kvstore_t *kvs,
    const char *name,
    const range_t *dst) {
    const any_t *any = kvstore_get(kvs, name);
    if (!any) { WARN("could not find"); return false; }
    const range_t range = any_get_bytes(any);
    if (range.size != dst->size) {
        WARN("bad size: %d/%d", range.size, dst->size);
        return false;
    }
    memcpy(dst->ptr, range.ptr, dst->size);
    return true;
}

#define DEFINE_PRIMITIVE_GET_SET(T, t_name)                                     \
    bool kvstore_get_##t_name(                                                 \
        const kvstore_t *kvstore,                                              \
        const char *name,                                                      \
        T *out) {                                                              \
        const any_t *any = kvstore_get(kvstore, name);                         \
        return any && any_get_##t_name(any, out);                              \
    }                                                                          \
                                                                               \
    T kvstore_get_##t_name##_or_default(                                       \
        const kvstore_t *kvstore,                                              \
        const char *name,                                                      \
        T _default) {                                                          \
        T res;                                                                 \
        return kvstore_get_##t_name(kvstore, name, &res) ? res : _default;     \
    }                                                                          \
                                                                               \
    void kvstore_set_##t_name(                                                 \
        kvstore_t *kvstore,                                                    \
        const char *name,                                                      \
        T value) {                                                             \
        any_t any;                                                             \
        any_init(&any, thread_scratch());                                      \
        any_set_##t_name(&any, value);                                         \
        kvstore_set(kvstore, name, &any);                                      \
    }

DEFINE_PRIMITIVE_GET_SET(int, int)
DEFINE_PRIMITIVE_GET_SET(char, char)
DEFINE_PRIMITIVE_GET_SET(bool, bool)
DEFINE_PRIMITIVE_GET_SET(u8, u8)
DEFINE_PRIMITIVE_GET_SET(u16, u16)
DEFINE_PRIMITIVE_GET_SET(u32, u32)
DEFINE_PRIMITIVE_GET_SET(u64, u64)
DEFINE_PRIMITIVE_GET_SET(i8, i8)
DEFINE_PRIMITIVE_GET_SET(i16, i16)
DEFINE_PRIMITIVE_GET_SET(i32, i32)
DEFINE_PRIMITIVE_GET_SET(i64, i64)
DEFINE_PRIMITIVE_GET_SET(f32, f32)
DEFINE_PRIMITIVE_GET_SET(f64, f64)
DEFINE_PRIMITIVE_GET_SET(vec2s, vec2)
DEFINE_PRIMITIVE_GET_SET(ivec2s, ivec2)

#undef DEFINE_PRIMITIVE_GET_SET
#endif // ifdef UTIL_IMPL
