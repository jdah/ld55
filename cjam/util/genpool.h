#pragma once

#include "macros.h"
#include "types.h"

typedef struct allocator allocator_t;

// fixed-sized struct (T) pool with generational indices
typedef struct genpool_s {
    allocator_t *allocator;

    // each header has a "header" of its generation + its linked list index
    // (sizeof(T) * size) + (sizeof(genpool_header_t) * size)
    void *data;

    // current size / total capacity
    i32 size, capacity;

    // sizeof(T)
    i32 t_size;

    // free linked list head, -1 if invalid/empty
    i32 free_head;
} genpool_t;

// header for a genpool entry
typedef struct {
    i32 gen;
    i32 next: 31;
    bool live: 1;
} genpool_header_t;

// generational index into genpool
typedef struct {
    i32 index, gen;
} genpool_id_t;

// null id
#define GENPOOL_ID_NULL ((genpool_id_t) { 0 })

void genpool_init(genpool_t *p, allocator_t *a, i32 capacity, i32 t_size);

void genpool_destroy(genpool_t *p);

// try get element of type T_ with specified id_ from genpool
#define genpool_try_get(T_, p_, id_) \
    ((T_*) genpool_try_get_voidp((p_), (id_)))

// try get element with specified id from genpool, returns NULL if invalid
void *genpool_try_get_voidp(const genpool_t *p, genpool_id_t id);

// try to get element with specified index (generationless!), returns NULL if
// not present
void *genpool_try_get_index(const genpool_t *p, i32 index);

// get header header for specified index, returns NULL if invalid
genpool_header_t *genpool_header(const genpool_t *p, i32 index);

// returns true if specified id is valid
bool genpool_valid(const genpool_t *p, genpool_id_t id);

// allocate new element of type T_, return pointer
#define genpool_add(T_, p_) ((T_*) genpool_add_voidp((p_)))

// allocate new element, return pointer
void *genpool_add_voidp(genpool_t *p);

// remove element with specified id, crashes (!) if id is invalid
void genpool_remove(genpool_t *p, genpool_id_t id);

// gets id of pointer into genpool, crashes if invalid (!)
genpool_id_t genpool_id_of(const genpool_t *p, void *ptr);

// gets next valid index in genpool after specified index
// returns -1 if no valid index
// if "index" == -1, finds first valid index
i32 genpool_next_index(const genpool_t *p, i32 index);

#define _genpool_each(T_, l_, it_, pname_, itname_)                     \
    typedef struct { i32 i; _T *el; } itname_;                          \
    typeof((_l)) pname_ = (l_);                                         \
    for (itname_ it_ = ({                                               \
            const i32 index_ = genpool_next_index(pname_, -1);          \
            (itname_) { index_, genpool_try_get_index(pname_, index_) };\
        };                                                              \
        it_.i != -1;                                                    \
        ({                                                              \
            it_.i = genpool_next_index(pname_, -1);                     \
            it_.el = genpool_try_get_index(pname_, it_.i);              \
        }))

#define genpool_each(T_, l_, it_)                                       \
    _genpool_each(                                                      \
        T_,                                                             \
        l_,                                                             \
        it_,                                                            \
        CONCAT(pp_, __COUNTER__),                                      \
        CONCAT(pi_, __COUNTER__))

#ifdef UTIL_IMPL

#include "alloc.h"

static void *genpool_data_ptr(const genpool_t *p) {
    return p->data;
}

static genpool_header_t *genpool_headers_ptr(const genpool_t *p) {
    return (genpool_header_t*) (((u8*) p->data) + (p->capacity * p->t_size));
}

void genpool_init(genpool_t *p, allocator_t *a, i32 capacity, i32 t_size) {
    // ensure alignment
    t_size = max(t_size, MAX_ALIGN);

    *p = (genpool_t) {
        .allocator = a,
        .data =
            mem_alloc(
                a,
                (sizeof(genpool_header_t) * capacity) + (t_size * capacity)),
        .size = 0,
        .capacity = capacity,
        .t_size = t_size,
        .free_head = 0, // free list starts from 0
    };

    // add everything to the free list
    genpool_header_t *headers = genpool_headers_ptr(p);
    for (i32 i = 0; i < capacity; i++) {
        headers[i].live = false;
        headers[i].next = i == (capacity - 1) ? -1 : (i + 1);
    }
}

void genpool_destroy(genpool_t *p) {
    mem_free(p->allocator, p->data);
    *p = (genpool_t) {};
}

void *genpool_try_get_voidp(const genpool_t *p, genpool_id_t id) {
    if (id.index < 0 || id.index >= p->capacity) {
        return NULL;
    }

    genpool_header_t *header = genpool_header(p, id.index);
    if (!header->live || header->gen != id.gen) {
        return NULL;
    }

    return ((u8*) genpool_data_ptr(p)) + (id.index * p->t_size);
}

void *genpool_try_get_index(const genpool_t *p, i32 index) {
    if (index < 0 || index >= p->capacity) {
        return NULL;
    }

    genpool_header_t *header = genpool_header(p, index);
    if (!header->live) {
        return NULL;
    }

    return ((u8*) genpool_data_ptr(p)) + (index * p->t_size);
}

genpool_header_t *genpool_header(const genpool_t *p, i32 index) {
    if (index < 0 || index >= p->capacity) { return NULL; }
    return &genpool_headers_ptr(p)[index];
}

bool genpool_valid(const genpool_t *p, genpool_id_t id) {
    return !!genpool_try_get_voidp(p, id);
}

void *genpool_add_voidp(genpool_t *p) {
    if (p->size == p->capacity) {
        WARN("genpool @ %p is out of space (cap. %d)", p, p->capacity);
        return NULL;
    }

    ASSERT(p->free_head != -1);

    const i32 i = p->free_head;

    genpool_header_t *header = genpool_header(p, i);
    ASSERT(!header->live);
    header->live = true;

    p->free_head = header->next;
    p->size++;

    return ((u8*) genpool_data_ptr(p)) + (i * p->t_size);
}

void genpool_remove(genpool_t *p, genpool_id_t id) {
    ASSERT(genpool_valid(p, id));

    genpool_header_t *header = genpool_header(p, id.index);
    ASSERT(header->live);
    header->live = false;

    // move onto front of free list
    if (p->free_head != -1) {
        genpool_header(p, p->free_head)->next = id.index;
    }
    header->next = p->free_head;

    p->free_head = id.index;
    p->size--;
}

genpool_id_t genpool_id_of(const genpool_t *p, void *ptr) {
    void *data = genpool_data_ptr(p);
    const uintptr_t
        iptr = (uintptr_t) ptr,
        first = (uintptr_t) data,
        last = first + ((p->capacity - 1) * p->t_size);

    ASSERT(
        iptr >= first
        && iptr <= last
        && ((iptr - first) % p->t_size) == 0);

    const i32 i = (iptr - first) / p->t_size;
    const genpool_header_t *header = genpool_header(p, i);

    return (genpool_id_t) { .index = i, .gen = header->gen };
}

i32 genpool_next_index(const genpool_t *p, i32 index) {
    do {
        index++;
    } while(index < p->capacity && !genpool_header(p, index)->live);

    return index == p->capacity ? -1 : index;
}
#endif // ifdef UTIL_IMPL
