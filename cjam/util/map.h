#pragma once

// simple linear probing hash-map implementation
// can be used with value size == 0 as a set
//
// some ideas for optimization:
// * profile for good load factors
// * instead of f_keycmp-ing with byte keys, or even small byte keys, just do
//   the comparison inline
// * same as above, try avoiding memcpying small values around

#include <string.h>
#include <stdlib.h>

#include "alloc.h"
#include "hash.h"
#include "types.h"
#include "assert.h"
#include "dynlist.h"

typedef hash_t (*map_hash_f)(map_t*, const void*);
typedef void *(*map_dup_f)(map_t*, void*);
typedef void (*map_free_f)(map_t*, void*);
typedef int (*map_cmp_f)(map_t*, const void*, const void*);

typedef struct map_entry {
    struct {
        bool used   : 1;
        u16 hash    : 15;
        u16 dist    : 16;
    };
} map_entry_t;

typedef struct map {
    allocator_t *allocator;

    map_hash_f f_hash;
    map_cmp_f f_keycmp;
    map_free_f f_keyfree, f_valfree;

    int key_size, value_size, used, capacity;

    // backing data: stored entries + keys + values + spares (2x key + 2x value)
    void *data;

    // arbitrary extra data, for user usage in function parameters
    void *userdata;

    // if true, map will not reshash
    bool rehash_locked: 1;

    // current spare index (only 0 or 1 since spare stores 2 values)
    u8 spare : 1;

    // current prime
    u8 prime;
} map_t;

// hash functions
hash_t map_hash_bytes(map_t *m, const void *p);
hash_t map_hash_str(map_t*, const void *p);
hash_t map_hash_inplace_str(map_t*, const void *p);

// compare functions
int map_cmp_bytes(map_t *m, const void *p, const void *q);
int map_cmp_str(map_t*, const void *p, const void *q);
int map_cmp_inplace_str(map_t *m, const void *p, const void *q);

// default map_free_fn implemented with stdlib's "free()", assumes values are
// pointers into heap memory
void map_default_free(map_t*, void *p);

// map_free_fn which uses map_t::allocator to free pointers
void map_allocator_free(map_t*, void *p);

// free for dynlists
void map_free_dynlist(map_t*, void *p);

// internal usage only
int _map_insert(map_t *map, const void *key, const void *value);

// internal usage only
int _map_find_pos(const map_t *map, const void *key);

// internal usage only
void _map_remove_at(map_t *map, int pos);

// remove key from map, crashes if not present
void map_remove(map_t *map, const void *key);

// remove but copy value
void map_remove_copy(map_t *map, const void *key, void *out);

// try to remove key from map, returns false if key was not present
bool map_try_remove(map_t *map, const void *key);

// map_try_remove but copy value if present
bool map_try_remove_copy(map_t *map, const void *key, void *out);

// create new map
void map_init(
    map_t *map,
    allocator_t *allocator,
    int key_size,
    int value_size,
    map_hash_f f_hash,
    map_cmp_f f_keycmp,
    map_free_f f_keyfree,
    map_free_f f_valfree,
    void *userdata);

// destroy map
void map_destroy(map_t *map);

// clear map of all keys and values
void map_clear(map_t *map);

// returns true if map has been initialized
bool map_valid(const map_t *map);

// true if map contains key
bool map_contains(const map_t *map, const void *key);

// get heap footprint of map
int map_footprint(const map_t *map);

M_INLINE map_entry_t *_map_entries(const map_t *map) {
    return map->data;
}

M_INLINE void *_map_keys(const map_t *map) {
    return
        map->data
            + round_up_to_mult(
                map->capacity * sizeof(map_entry_t), MAX_ALIGN);
}

M_INLINE void *_map_values(const map_t *map) {
    if (map->value_size == 0) { return NULL; }

    return
        map->data
            + round_up_to_mult(
                map->capacity * sizeof(map_entry_t), MAX_ALIGN)
            + round_up_to_mult(
                map->capacity * map->key_size, MAX_ALIGN);
}

// pointer to spare data (2x key + 2x value)
M_INLINE void *_map_spares(const map_t *map) {
    return
        map->data
            + round_up_to_mult(
                map->capacity * sizeof(map_entry_t), MAX_ALIGN)
            + round_up_to_mult(
                map->capacity * map->key_size, MAX_ALIGN)
            + round_up_to_mult(
                map->capacity * map->value_size, MAX_ALIGN);
}

M_INLINE void *_map_key_at(const map_t *map, int index) {
    return _map_keys(map) + (index * map->key_size);
}

M_INLINE void *_map_value_at(const map_t *map, int index) {
    void *values = _map_values(map);
    return values ? (values + (index * map->value_size)) : NULL;
}

// insert (k, v) into map, replacing value if it is already present, returns
// pointer to value with same type as _v
#define map_insert(_m, _k, _v) ({                                           \
        map_t *__m = (_m);                                                  \
        const M_UNUSED int __n = _map_insert(__m, (_k), (_v));              \
        (typeof((_v))) _map_value_at(__m, __n);                             \
    })

// returns _T *value, NULL if not present
#define _map_get3(_T, _m, _k) ({                                            \
        const map_t *__m = (_m);                                            \
        const int _i = _map_find_pos(__m, (_k));                            \
        _i == -1 ? NULL : (typeof_pointer_to(_T)) _map_value_at(__m, _i);   \
    })

// returns void **value, NULL if not present
#define _map_get2(_m, _k) ({                                                \
        const map_t *__m = (_m);                                            \
        const int _i = _map_find_pos(__m, (_k));                            \
        _i == -1 ? NULL : _map_value_at(__m, _i);                           \
    })

// map_get(map, key)/map_find(TYPE, map, key) -> ptr to value, NULL if not found
#define map_get(...) VMACRO(_map_get, __VA_ARGS__)

// get next used entry from map index _i. returns _m->capacity at map end
#define map_next(_m, _i) ({                                                 \
        typeof(_m) __m = (_m);                                              \
        int _j = _i;                                                        \
        _j++;                                                               \
        while (_j < __m->capacity && !_map_entries(__m)[_j].used) { _j++; } \
        ASSERT(_j >= __m->capacity || _map_entries(__m)[_j].used);          \
        _j;                                                                 \
    })

#define _map_each_impl(_KT, _VT, _m, _it, _itname)                          \
    typedef struct {                                                        \
        typeof(_m) __m; int __i; _KT *key; _VT *value; } _itname;           \
    for (_itname _it = {                                                    \
            .__m = (_m),                                                    \
            .__i = map_next(_it.__m, -1),                                   \
            .key = _it.__i < _it.__m->capacity ?                            \
                _map_key_at(_it.__m, _it.__i) : NULL,                       \
            .value = _it.__i < _it.__m->capacity ?                          \
                _map_value_at(_it.__m, _it.__i) : NULL,                     \
         }; _it.__i < _it.__m->capacity;                                    \
         _it.__i = map_next(_it.__m, _it.__i),                              \
        _it.key =                                                           \
            _it.__i < _it.__m->capacity ?                                   \
                _map_key_at(_it.__m, _it.__i) : NULL,                       \
        _it.value =                                                         \
            _it.__i < _it.__m->capacity ?                                   \
                _map_value_at(_it.__m, _it.__i) : NULL)

// map_each(KEY_TYPE, VALUE_TYPE, *map, it_name)
#define map_each(_KT, _VT, _m, _it)                                         \
    _map_each_impl(                                                         \
        _KT,                                                                \
        _VT,                                                                \
        _m,                                                                 \
        _it,                                                                \
        CONCAT(_mei, __COUNTER__))

// true if map is empty
M_INLINE bool map_empty(const map_t *map) { return map->used == 0; }

// number of elements in map
M_INLINE int map_size(const map_t *map) { return map->used; }

#ifdef UTIL_IMPL
#include "assert.h"

#include <stdlib.h>
#include <string.h>

// planetmath.org/goodhashtableprimes
static const u32 PRIMES[] = {
    11, 53, 97, 193, 389, 769, 1543, 3079, 6151, 12289, 24593, 49157, 98317,
    196613, 393241, 786433, 1572869, 3145739, 6291469, 12582917, 25165843,
    50331653, 100663319, 201326611, 402653189, 805306457, 1610612741
};

// load factors at which rehashing happens
#define MAP_LOAD_HIGH 0.85
#define MAP_LOAD_LOW 0.15

#define MAP_MAX_DISTANCE U16_MAX

// mask for map_internal_entry_t::hash
#define MAP_STORED_HASH_MASK ((1 << 15) - 1)

void map_init(
    map_t *map,
    allocator_t *allocator,
    int key_size,
    int value_size,
    map_hash_f f_hash,
    map_cmp_f f_keycmp,
    map_free_f f_keyfree,
    map_free_f f_valfree,
    void *userdata) {
    *map = (map_t) { 0 };

    map->allocator = allocator;
    map->key_size = key_size;
    map->value_size = value_size;
    map->f_hash = f_hash;
    map->f_keycmp = f_keycmp;
    map->f_keyfree = f_keyfree;
    map->f_valfree = f_valfree;
    map->userdata = userdata;
}

static void _map_internal_destroy(map_t *map) {
    if (!map->data) {
        return;
    }

    for (int i = 0; i < map->capacity; i++) {
        if (_map_entries(map)[i].used) {
            if (map->f_keyfree) {
                map->f_keyfree(map, _map_key_at(map, i));
            }

            if (map->f_valfree && map->value_size) {
                map->f_valfree(map, _map_value_at(map, i));
            }
        }
    }

    mem_free(map->allocator, map->data);

    map->used = 0;
    map->capacity = 0;
    map->prime = 0;
    map->data = NULL;
    map->spare = 0;
    map->rehash_locked = false;
}

void map_destroy(map_t *map) {
    _map_internal_destroy(map);
}

bool map_valid(const map_t *map) {
    return !!map->allocator;
}

enum {
    REHASH_NONE   = 0 << 0,
    REHASH_GROW   = 1 << 0,
    REHASH_SHRINK = 1 << 1,
    REHASH_FORCE  = 1 << 2,
};

// returns true on rehash
static bool map_rehash_or_alloc(map_t *map, int flags) {
    if (map->rehash_locked) { return false; }

    const int old_capacity = map->capacity;

    const f32 load =
        map->capacity == 0 ? 0.0f : (map->used / (f32) (map->capacity));

    const bool
        force = !!(flags & REHASH_FORCE),
        low = (load < MAP_LOAD_LOW) && (flags & REHASH_SHRINK),
        high = (load > MAP_LOAD_HIGH) && (flags & REHASH_GROW);

    if (!map->data) {
        map->prime = 0;
        goto alloc;
    }

    if (!force
        && ((!low && !high)
            || (low && map->prime == 0)
            || (high && map->prime == ARRLEN(PRIMES) - 1))) {
        return false;
    }

    ASSERT(
        force
            || (high && map->prime != ARRLEN(PRIMES) - 1)
            || (low && map->prime != 0),
        "attempted rehash but cannot expand/shrink");

    if (low && map->prime != 0) {
        map->prime--;
    } else if (force || (high && map->prime != (ARRLEN(PRIMES) - 1))) {
        map->prime++;
    }

alloc:;
    map_entry_t *old_entries = _map_entries(map);
    void *old_keys = _map_keys(map), *old_values = _map_values(map);
    void *old_data = map->data;

    ASSERT(map->prime < (int) ARRLEN(PRIMES), "out of memory");
    map->capacity = PRIMES[map->prime];

    // recalculated on the fly with inserts
    map->used = 0;

    // size = (entry + key + value) * capacity + (2x key + 2x value)
    const int
        size_entries =
            round_up_to_mult(map->capacity * sizeof(map_entry_t), MAX_ALIGN),
        size_data =
            size_entries
                + round_up_to_mult(map->capacity * map->key_size, MAX_ALIGN)
                + round_up_to_mult(map->capacity * map->value_size, MAX_ALIGN)
                + round_up_to_mult(map->key_size * 2, MAX_ALIGN)
                + round_up_to_mult(map->value_size * 2, MAX_ALIGN);

    map->data = mem_alloc(map->allocator, size_data);
    memset(_map_entries(map), 0, size_entries);

    for (int i = 0; i < map->capacity; i++) {
        ASSERT(!_map_entries(map)[i].used);
    }

    // re-insert all entries
    if (old_data) {
        map->rehash_locked = true;
        for (int i = 0; i < old_capacity; i++) {
            if (!old_entries[i].used) { continue; }

            void *key = old_keys + (i * map->key_size),
                 *value =
                     old_values ? (old_values + (i * map->value_size)) : NULL;

            _map_insert(map, key, value);
        }

        mem_free(map->allocator, old_data);
        map->rehash_locked = false;
    }

    return true;
}

int _map_insert(
    map_t *map,
    const void *key,
    const void *value) {
    const void *original_key = key;

    if (!map->data) {
        map_rehash_or_alloc(map, REHASH_NONE);
    }

    hash_t
        hash = map->f_hash(map, key),
        hash_bits = hash & MAP_STORED_HASH_MASK;

    int pos = hash % map->capacity, dist = 0, result_pos = -1;

    bool is_new = true;

    while (true) {
        map_entry_t *entry = &_map_entries(map)[pos];
        if (!entry->used) {
            if (is_new) { result_pos = pos; }

            *entry = (map_entry_t) {
                .used = true,
                .dist = dist,
                .hash = hash_bits
            };
            memcpy(_map_key_at(map, pos), key, map->key_size);
            if (map->value_size) {
                memcpy(_map_value_at(map, pos), value, map->value_size);
            }
            break;
        } else if (
            entry->dist == dist
                && entry->hash == hash_bits
                && !map->f_keycmp(map, _map_key_at(map, pos), key)) {
            if (is_new) { result_pos = pos; }
            is_new = false;

            // entry already exists, replace value
            if (map->f_keyfree) {
                map->f_keyfree(map, _map_key_at(map, pos));
            }

            if (map->f_valfree && map->value_size) {
                map->f_valfree(map, _map_value_at(map, pos));
            }

            memcpy(_map_key_at(map, pos), key, map->key_size);
            if (map->value_size) {
                memcpy(_map_value_at(map, pos), value, map->value_size);
            }

            break;
        } else if (entry->dist < dist) {
            if (is_new) {
                map->used++;
                result_pos = pos;
                is_new = false;
            }

            // replace this element, keep probing with new element
            map_entry_t old_entry = *entry;
            *entry = (map_entry_t) {
                .used = true,
                .dist = dist,
                .hash = hash_bits
            };

#define SPARE_KEY(i) (_map_spares(map) + (((int) i) * map->key_size))
#define SPARE_VALUE(i)                                                         \
    (_map_spares(map)                                                          \
        + round_up_to_mult(map->key_size * 2, MAX_ALIGN)                       \
        + (((int) i) * map->value_size))

            // do some swaps and continue trying to insert
            // spare <- map_key/value_at(pos)
            // map_key/value_at(pos) <- key/value
            // key/value <- spare
            memcpy(
                SPARE_KEY(map->spare),
                _map_key_at(map, pos),
                map->key_size);

            if (map->value_size) {
                memcpy(
                    SPARE_VALUE(map->spare),
                    _map_value_at(map, pos),
                    map->value_size);
            }

            memcpy(_map_key_at(map, pos), key, map->key_size);

            if (map->value_size) {
                memcpy(_map_value_at(map, pos), value, map->value_size);
            }

            dist = old_entry.dist;
            hash_bits = old_entry.hash;
            key = SPARE_KEY(map->spare);
            value = map->value_size ? SPARE_VALUE(map->spare) : NULL;

            // flip spare for next usage
            map->spare = 1 - map->spare;

#undef SPARE_KEY
#undef SPARE_VALUE
        }

        // rehash if we have a greater distance than storage allows
        dist++;

        if (dist >= MAP_MAX_DISTANCE) {
            map_rehash_or_alloc(map, REHASH_FORCE);
            _map_insert(map, key, value);
            result_pos = _map_find_pos(map, original_key);
            is_new = false;
            goto done;
        }

        // check next location
        pos = (pos + 1) % map->capacity;
    }

    // rehash if necessary
    if (map_rehash_or_alloc(map, REHASH_GROW)) {
        // recompute pos as it may have moved on rehash
        result_pos = _map_find_pos(map, original_key);
        is_new = false;
    }

done:
    if (is_new) { map->used++; }
    return result_pos;
}

int _map_find_pos(const map_t *map, const void *key) {
    if (!map->data) { return -1; }

    const hash_t hash = map->f_hash((map_t*) map, key);
    int pos = hash % map->capacity, res = -1;
    hash_t hashbits = hash & MAP_STORED_HASH_MASK;
    const map_entry_t *entries = _map_entries(map);

    while (true) {
        if (!entries[pos].used) {
            break;
        }

        if (entries[pos].hash == hashbits
                && !map->f_keycmp(
                        (map_t*) map, _map_key_at((map_t*) map, pos), key)) {
            res = pos;
            break;
        }

        pos = (pos + 1) % map->capacity;
    }

    return res;
}

void _map_remove_at(map_t *map, int pos) {
    int next_pos = 0;
    ASSERT(pos < map->capacity, "could not remove key at pos %" PRIint, pos);

    map_entry_t *entries = _map_entries(map);
    map_entry_t *entry = &entries[pos];
    ASSERT(entry->used);

    if (map->f_keyfree) {
        map->f_keyfree(map, _map_key_at(map, pos));
    }

    if (map->f_valfree && map->value_size) {
        map->f_valfree(map, _map_value_at(map, pos));
    }

    map->used--;

    // mark empty
    entry->used = false;

    // shift further entries back by one
    while (true) {
        next_pos = (pos + 1) % map->capacity;

        // stop when unused entry is found or dist is 0
        if (!entries[next_pos].used
            || entries[next_pos].dist == 0) {
            break;
        }

        // copy next pos into current
        entries[pos] = entries[next_pos];
        entries[pos].dist--;

        memcpy(
            _map_key_at(map, pos),
            _map_key_at(map, next_pos),
            map->key_size);

        if (map->value_size) {
            memcpy(
                _map_value_at(map, pos),
                _map_value_at(map, next_pos),
                map->value_size);
        }

        // mark next entry as empty
        _map_entries(map)[next_pos].used = false;

        pos = next_pos;
    }

    // might rehash because of low load factor
    map_rehash_or_alloc(map, REHASH_SHRINK);
}

void map_remove(map_t *map, const void *key) {
    map_remove_copy(map, key, NULL);
}

void map_remove_copy(map_t *map, const void *key, void *out) {
    const hash_t hash = map->f_hash(map, key);
    const int pos = _map_find_pos(map, key);
    ASSERT(pos != -1, "could not find key with hash %" PRIint, hash);
    if (out && map->value_size) {
        memcpy(out, _map_value_at(map, pos), map->value_size);
    }
    _map_remove_at(map, pos);
}

bool map_try_remove(map_t *map, const void *key) {
    return map_try_remove_copy(map, key, NULL);
}

bool map_try_remove_copy(map_t *map, const void *key, void *out) {
    const int pos = _map_find_pos(map, key);
    if (pos != -1) {
        if (out && map->value_size) {
            memcpy(out, _map_value_at(map, pos), map->value_size);
        }

        _map_remove_at(map, pos);
    }
    return pos != -1;
}

bool map_contains(const map_t *map, const void *key) {
    return _map_find_pos(map, key) != -1;
}

void map_clear(map_t *map) {
    _map_internal_destroy(map);
}

int map_footprint(const map_t *map) {
    const int
        size_entries =
            round_up_to_mult(map->capacity * sizeof(map_entry_t), MAX_ALIGN),
        size_keys =
            round_up_to_mult(map->capacity * map->key_size, MAX_ALIGN),
        size_values =
            round_up_to_mult(map->capacity * map->value_size, MAX_ALIGN),
        size_data = size_entries + size_keys + size_values;

    return size_data
        + round_up_to_mult(2 * map->key_size, MAX_ALIGN)
        + round_up_to_mult(2 * map->value_size, MAX_ALIGN);
}

hash_t map_hash_bytes(map_t *m, const void *p) {
    return hash_add_bytes(0xDEADBEEF, p, m->key_size);
}

hash_t map_hash_str(map_t*, const void *p) {
    return hash_add_str(0xDEADBEEF, *(const char**) p);
}

hash_t map_hash_inplace_str(map_t*, const void *p) {
    return hash_add_str(0xDEADBEEF, p);
}

// compare functions
int map_cmp_bytes(map_t *m, const void *p, const void *q) {
    return memcmp(p, q, m->key_size);
}

int map_cmp_str(map_t*, const void *p, const void *q) {
    return strcmp(*(const char**) p, *(const char**) q);
}

int map_cmp_inplace_str(map_t *m, const void *p, const void *q) {
    return strncmp(p, q, m->key_size);
}

// default map_free_fn implemented with stdlib's "free()", assumes values are
// pointers into heap memory
void map_default_free(map_t*, void *p) {
    free(*(void**) p);
}

void map_allocator_free(map_t *m, void *p) {
    mem_free(m->allocator, *(void**) p);
}

// free for dynlists
void map_free_dynlist(map_t*, void *p) {
    dynlist_destroy(*(DYNLIST(void)*) p);
}
#endif // ifdef UTIL_IMPL
