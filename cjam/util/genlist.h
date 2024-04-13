#pragma once

#include "types.h"
#include "blklist.h"

#define genlist_handle_BITS 20 // 2^20 = 1048576
#define genlist_GEN_BITS   12 // 2^12 = 4096

STATIC_ASSERT(genlist_handle_BITS + genlist_GEN_BITS == 32);

typedef u16 GENLIST_GEN_TYPE;
STATIC_ASSERT(genlist_GEN_BITS <= sizeof(GENLIST_GEN_TYPE) * 8);

// generational list
typedef struct genlist {
    blklist_t data;

    // generation for each slot in data list, never shrinks
    DYNLIST(GENLIST_GEN_TYPE) gens;
} genlist_t;

typedef struct {
    u32 index: genlist_handle_BITS;
    GENLIST_GEN_TYPE gen: genlist_GEN_BITS;
} genlist_handle_t;

#define genlist_handle_NULL ((genlist_handle_t) { 0, 0 })

// returns true if two indices are equal
M_INLINE bool genlist_handle_eq(genlist_handle_t a, genlist_handle_t b) {
    return a.gen == b.gen && a.index == b.index;
}

// returns true if idx is genlist_handle_NULL
M_INLINE bool genlist_handle_is_null(genlist_handle_t idx) {
    return !idx.gen && !idx.index;
}

void genlist_init(
    genlist_t *l,
    allocator_t *a,
    usize block_size,
    usize t_size);

void genlist_destroy(genlist_t *l);

// clears list, keeping generations
void genlist_clear(genlist_t *l);

// resets list, ALSO ZEROING GENERATIONS (!) this invalidates all indices
void genlist_reset(genlist_t *l);

// add element to list, returns pointer to slot + optional index
void *genlist_add_voidp(genlist_t *l, genlist_handle_t *index_out);

// try convert idx -> ptr, returns NULL if not present
void *genlist_try_ptr_voidp(const genlist_t *l, genlist_handle_t idx);

// returns true if idx is present
bool genlist_present(const genlist_t *l, genlist_handle_t idx);

// remove element at idx, crashes if not present
void genlist_remove(genlist_t *l, genlist_handle_t idx);

// try to remove element at idx if present, returns true on success
bool genlist_try_remove(genlist_t *l, genlist_handle_t idx);

// try to get index from raw index into list, returns genlist_handle_NULL if not
// present
genlist_handle_t genlist_handle_of_index(const genlist_t *l, u32 index);

// try to get index from pointer into list, returns genlist_handle_NULL if not
// present
genlist_handle_t genlist_handle_of_ptr(const genlist_t *l, const void *p);

// try to get void* from raw index
void *genlist_try_ptr_from_index_voidp(const genlist_t *l, u32 index);

// (type, list, index_out)
#define _genlist_add3(T, l, i) ((T*) (genlist_add_voidp((l), (i))))

// (type, list)
#define _genlist_add2(T, l) ((T*) (genlist_add_voidp((l), NULL)))

// add an element of type T, returns pointer to element slot
// vmacro: (type, list) or (type, list, index_out)
#define genlist_add(...) VMACRO(_genlist_add, __VA_ARGS__)

// try to get T* from index i
#define genlist_try_ptr(T, l, i) ((T*) (genlist_try_ptr_voidp((l), (i))))

// try to get T* from genlist_handle_t i
#define genlist_try_ptr_from_index(T, l, i) \
    ((T*) (genlist_try_ptr_from_index_voidp((l), (i))))

// iterate over a genlist
#define genlist_each(T, l, ...) blklist_each(T, &(l)->data, __VA_ARGS__)

#ifdef UTIL_IMPL

void genlist_init(
    genlist_t *l,
    allocator_t *a,
    usize block_size,
    usize t_size) {
    blklist_init(&l->data, a, block_size, t_size);
    dynlist_init(l->gens, a);
}

void genlist_destroy(genlist_t *l) {
    blklist_destroy(&l->data);
    dynlist_destroy(l->gens);
    *l = (genlist_t) { 0 };
}

void genlist_clear(genlist_t *l) {
    blklist_clear(&l->data);
}

void genlist_reset(genlist_t *l) {
    blklist_clear(&l->data);
    dynlist_resize(l->gens, 0);
}

void *genlist_add_voidp(genlist_t *l, genlist_handle_t *index_out) {
    void *result = blklist_add_voidp(&l->data);
    const i32 index = blklist_index_of(&l->data, result);

    // ensure capacity in indices
    if (index >= dynlist_size(l->gens)) {
        const i32 old_size = dynlist_size(l->gens);
        dynlist_resize(l->gens, index + 1);

        // initialize to gen 0
        if (old_size > 0) {
            memset(
                &l->gens[old_size],
                0,
                ((index + 1) - old_size) * sizeof(l->gens[0]));
        }
    }

    if (l->gens[index] == 0) {
        l->gens[index] = 1;
    }

    if (index_out) {
        *index_out =
            (genlist_handle_t) { .gen = l->gens[index], .index = index };
    }

    return result;
}

void *genlist_try_ptr_voidp(const genlist_t *l, genlist_handle_t idx) {
    if (idx.index >= (u32) dynlist_size(l->gens)
        || l->gens[idx.index] != idx.gen) {
        return NULL;
    }

    return blklist_try_ptr_voidp(&l->data, idx.index);
}

bool genlist_present(const genlist_t *l, genlist_handle_t idx) {
    return !!genlist_try_ptr_voidp(l, idx);
}

void genlist_remove(genlist_t *l, genlist_handle_t idx) {
    // TODO(refactor): remove in release / use DEBUG_ASSERT(...)
    ASSERT(genlist_present(l, idx));

    blklist_remove(&l->data, idx.index);

    if (l->gens[idx.index] == (1 << genlist_GEN_BITS) - 1) {
        l->gens[idx.index] = 0;
    } else {
        l->gens[idx.index]++;
    }
}

bool genlist_try_remove(genlist_t *l, genlist_handle_t idx) {
    if (!genlist_present(l, idx)) {
        return false;
    }

    genlist_remove(l, idx);
    return true;
}

genlist_handle_t genlist_handle_of_index(const genlist_t *l, u32 index) {
    if (!blklist_present(&l->data, index)) { return genlist_handle_NULL; }
    return (genlist_handle_t) { .gen = l->gens[index], .index = index };
}

void *genlist_try_ptr_from_index_voidp(const genlist_t *l, u32 index) {
    const genlist_handle_t idx = genlist_handle_of_index(l, index);
    return genlist_handle_is_null(idx) ? NULL : genlist_try_ptr_voidp(l, idx);
}

genlist_handle_t genlist_handle_of_ptr(const genlist_t *l, const void *p) {
    const i32 index = blklist_index_of(&l->data, p);
    if (index == -1) { return genlist_handle_NULL; }
    if (!blklist_present(&l->data, index)) { return genlist_handle_NULL; }
    return (genlist_handle_t) { .gen = l->gens[index], .index = index };
}

#endif // ifdef UTIL_IMPL
