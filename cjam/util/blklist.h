#pragma once

#include "bitmap.h"
#include "types.h"
#include "macros.h"
#include "alloc.h"

// implements blklist_t, a list which only supports "add at any index" and
// "remove at specific index" ops but maintains pointer AND index stability by
// allocating in blocks of N elements at a time
//
// internally implemented as a series of allocator_t allocated blocks prefixed
// each by an in-place bitmap_t keeping track of free indices

// header of a block layout (always just void* underlying)
// * aligned bitmap_t with enough space for bitmap_t and bytes for block_size
// * space for data, t_size * block_size
typedef struct {} blklist_block_t;

typedef struct blklist {
    allocator_t *allocator;

    // blocks - entries CAN BE NULL if block has been silently deallocated
    DYNLIST(blklist_block_t*) blocks;

    i32 size, capacity, block_size, t_size, good_block;
} blklist_t;

// initializes a blklist
void blklist_init(
    blklist_t *bl,
    allocator_t *al,
    usize block_size,
    usize t_size);

// destroys a blklist
void blklist_destroy(blklist_t *bl);

// clears a blklist
void blklist_clear(blklist_t *bl);

// add into list at any index, returns pointer to allocated space
void *blklist_add_voidp(blklist_t *bl);

// removes from list at index
void blklist_remove(blklist_t *bl, i32 index);

// gets index from pointer into list, -1 if invalid
i32 blklist_index_of(const blklist_t *bl, const void *p);

// returns true if index exists in list
bool blklist_present(const blklist_t *bl, i32 index);

// get pointer to element at index
void *blklist_ptr_voidp(const blklist_t *bl, i32 index);

// get pointer to element at index (NO BOUNDS CHECK!)
void *blklist_ptr_voidp_unsafe(const blklist_t *bl, i32 index);

// get pointer to element at index IF it is present, otherwise return NULL
void *blklist_try_ptr_voidp(const blklist_t *bl, i32 index);

// gets next valid index for list after i, -1 if no such index
// pass i == -1 to get the first valid index
i32 blklist_next_index(const blklist_t *bl, i32 i);

// get memory footpri32 of list in bytes
usize blklist_footprint(const blklist_t *bl);

// get overhead (amount of extra memory used compared to simple array) of list
// in bytes
usize blklist_overhead(const blklist_t *bl);

// insert new element at any index, returns pointer to new element
#define blklist_add(_T, _l) ((_T*) blklist_add_voidp((_l)))

// get pointer to element at index
#define blklist_ptr(_T, _l, _i) ((_T*) blklist_ptr_voidp((_l), (_i)))

// get pointer to element at index (NO BOUNDS CHECK!)
#define blklist_ptr_unsafe(_T, _l, _i) ((_T*) blklist_ptr_voidp_unsafe((_l), (_i)))

// get pointer to element at index if it is present, otherwise returns NULL
#define blklist_try_ptr(_T, _l, _i) ((_T*) blklist_try_ptr_voidp((_l), (_i)))

#define _blklist_each_impl(_T, _l, _it, _cond, _pname, _itname)           \
    typedef struct { i32 i; _T* el; } _itname;                            \
    typeof((_l)) _pname = (_l);                                           \
    for (_itname _it = ({                                                 \
            _itname _it = { .i = -1 };                                    \
            do {                                                          \
                _it.i = blklist_next_index(_pname, _it.i);                \
                _it.el = _it.i == -1 ? NULL : blklist_ptr_unsafe(_T, _l, _it.i); \
            } while (_it.i != -1 && !(_cond));                            \
            _it;                                                          \
         });                                                              \
         _it.i != -1 && _it.i < _pname->capacity;                         \
         ({                                                               \
            do {                                                          \
                _it.i = blklist_next_index(_pname, _it.i);                \
                _it.el = _it.i == -1 ? NULL : blklist_ptr_unsafe(_T, _l, _it.i); \
            } while (_it.i != -1 && !(_cond));                            \
         }))

#define _blklist_each4(_T, _l, _it, _cond)                                \
    _blklist_each_impl(                                                   \
        _T,                                                               \
        _l,                                                               \
        _it,                                                              \
        _cond,                                                            \
        CONCAT(_dlp, __COUNTER__),                                        \
        CONCAT(_dli, __COUNTER__))

#define _blklist_each3(_T, _l, _it)                                       \
    _blklist_each_impl(                                                   \
        _T,                                                               \
        _l,                                                               \
        _it,                                                              \
        true,                                                             \
        CONCAT(_dlp, __COUNTER__),                                        \
        CONCAT(_dli, __COUNTER__))

// usage: blklist_each(<type>, <list>, <it>, [start?], [end?]) { ... }
#define blklist_each(...) VMACRO(_blklist_each, __VA_ARGS__)

#ifdef UTIL_IMPL
#include "math.h"
#include "assert.h"
#include "dynlist.h"

void blklist_init(
    blklist_t *bl,
    allocator_t *al,
    usize block_size,
    usize t_size) {
    *bl = (blklist_t) {
        .allocator = al,
        .size = 0,
        .capacity = 0,
        .block_size = block_size,
        .t_size = round_up_to_mult(t_size, MAX_ALIGN),
        .good_block = -1,
        .blocks = dynlist_create(blklist_block_t*, al, 1),
    };
}

void blklist_destroy(blklist_t *bl) {
    dynlist_each(bl->blocks, it) {
        if (*it.el) {
            mem_free(bl->allocator, *it.el);
        }
    }
    dynlist_destroy(bl->blocks);

    *bl = (blklist_t) { 0 };
}

void blklist_clear(blklist_t *bl) {
    dynlist_each(bl->blocks, it) {
        if (*it.el) {
            mem_free(bl->allocator, *it.el);
        }
    }

    dynlist_resize(bl->blocks, 0);
    bl->good_block = -1;
    bl->size = 0;
    bl->capacity = 0;
}

M_INLINE bitmap_t *block_bits(const blklist_t *bl, blklist_block_t *block) {
    return (bitmap_t*) block;
}

M_INLINE void *block_data(const blklist_t *bl, blklist_block_t *block) {
    return
        &((u8*) block)[
            round_up_to_mult(
                sizeof(bitmap_t)
                    + BITMAP_SIZE_TO_BYTES(bl->block_size),
                MAX_ALIGN)];
}

void *blklist_add_voidp(blklist_t *bl) {
    blklist_block_t *block = NULL;

    // index of chosen block, internal free index into block
    i32 block_index = 0, i = -1;

    ASSERT(bl->good_block == -1 || bl->blocks[bl->good_block]);

    // do we have a good_block?
    if (bl->good_block != -1
        && (i =
                bitmap_find(
                    block_bits(bl, bl->blocks[bl->good_block]), 0, false))
                != -1) {
        // got good block and good i
        block_index = bl->good_block;
        block = bl->blocks[bl->good_block];
    } else {
        // scan from start of blocks
        dynlist_each(bl->blocks, it) {
            // check block is not NULL, may have been silently deallocated
            if (*it.el
                && ((i = bitmap_find(block_bits(bl, *it.el), 0, false)) != -1)) {
                block_index = it.i;
                block = *it.el;
                break;
            }
        }
    }

    // check if we need to add a block
    if (!block) {
        // allocate for bitmap + data
        block =
            mem_alloc(
                bl->allocator,
                round_up_to_mult(
                    sizeof(bitmap_t)
                        + BITMAP_SIZE_TO_BYTES(bl->block_size),
                    MAX_ALIGN)
                    + (bl->t_size * bl->block_size));

        // init and zero bitmap
        bitmap_init(block_bits(bl, block), NULL, bl->block_size);
        bitmap_fill(block_bits(bl, block), 0);

        // allocate first element
        i = 0;

        // check if there is free space (silently deallocated) for this block
        for (block_index = 0;
             block_index < dynlist_size(bl->blocks);
             block_index++) {
            if (!bl->blocks[block_index]) { break; }
        }

        if (block_index == dynlist_size(bl->blocks)) {
            bl->capacity += bl->block_size;

            // add block on end
            *dynlist_insert(bl->blocks, block_index) = block;
        } else {
            // assign to previously deallocated block
            bl->blocks[block_index] = block;
        }
    }

    ASSERT(block);
    ASSERT(i != -1);
    ASSERT(block_index >= 0);
    ASSERT(block_index < dynlist_size(bl->blocks));

    // this is a good block if this isn't the last index being allocated
    if (i != bl->block_size - 1) {
        bl->good_block = block_index;
    }

    bl->size++;

    ASSERT(!bitmap_get(block_bits(bl, block), i));
    bitmap_set(block_bits(bl, block), i);
    ASSERT(blklist_present(bl, i + (block_index * bl->block_size)));
    return block_data(bl, block) + ((i % bl->block_size) * bl->t_size);
}

void blklist_remove(blklist_t *bl, i32 index) {
    ASSERT(index >= 0 && index <= bl->capacity);

    const int
        block_index = index / bl->block_size,
        i_block = index % bl->block_size;

    ASSERT(block_index < dynlist_size(bl->blocks));

    blklist_block_t *block = bl->blocks[block_index];
    ASSERT(block);

    ASSERT(bitmap_get(block_bits(bl, block), i_block));
    bitmap_clr(block_bits(bl, block), i_block);

    bl->size--;

    // check if we can deallocate block
    if (bitmap_count(block_bits(bl, block), 1) == 0) {
        // remove block, zero place out - may not be able to shrink list
        // however
        mem_free(bl->allocator, block);
        bl->blocks[block_index] = NULL;

        // can only remove if this is the last block on the list
        if (block_index == dynlist_size(bl->blocks) - 1) {
            bl->capacity -= bl->block_size;
            dynlist_remove(bl->blocks, block_index);
        }

        if (bl->good_block == block_index) {
            bl->good_block = -1;
        }

        ASSERT(bl->good_block == -1 || bl->blocks[bl->good_block]);
    }
}

i32 blklist_index_of(const blklist_t *bl, const void *p) {
    dynlist_each(bl->blocks, it) {
        if (!*it.el) { continue; }

        void *data = block_data(bl, *it.el);
        if (p >= data && p < (data + (bl->block_size * bl->t_size))) {
            return
                (it.i * bl->block_size)
                    + (((int) (p - data)) / bl->t_size);
        }
    }

    return -1;
}

bool blklist_present(const blklist_t *bl, i32 index) {
    const i32 block_index = index / bl->block_size;

    if (index < 0
        || index >= bl->capacity
        || block_index >= dynlist_size(bl->blocks)
        || !bl->blocks[block_index]) {
        return false;
    }

    return
        bitmap_get(
            block_bits(bl, bl->blocks[block_index]),
            index % bl->block_size);
}

void *blklist_ptr_voidp(const blklist_t *bl, i32 index) {
    ASSERT(blklist_present(bl, index));

    return
        block_data(bl, bl->blocks[index / bl->block_size])
            + ((index % bl->block_size) * bl->t_size);
}

void *blklist_ptr_voidp_unsafe(const blklist_t *bl, i32 index) {
    return
        block_data(bl, bl->blocks[index / bl->block_size])
            + ((index % bl->block_size) * bl->t_size);
}

void *blklist_try_ptr_voidp(const blklist_t *bl, i32 index) {
    const i32 block_index = index / bl->block_size;

    if (index < 0
        || index >= bl->capacity
        || block_index >= dynlist_size(bl->blocks)) {
        return NULL;
    }

    blklist_block_t *block = bl->blocks[block_index];

    if (!block
        || !bitmap_get(block_bits(bl, block), index % bl->block_size)) {
        return NULL;
    }

    return block_data(bl, block) + ((index % bl->block_size) * bl->t_size);
}

i32 blklist_next_index(const blklist_t *bl, i32 index) {
    i32 block_index;

    if (index == -1) {
        // start searching at block 0
        block_index = -1;
        goto find_next;
    } else {
        block_index = index / bl->block_size;
    }

    if (block_index < 0 || block_index >= dynlist_size(bl->blocks)) {
        // index is invalid
        return -1;
    }

    i32 i;

    // check if we can advance in the same block -
    // * block must still exist
    // * must not be at last index of block
    // * must be able to find next present bit in block
    if (bl->blocks[block_index]
        && (index % bl->block_size) != bl->block_size - 1
        && (i =
            bitmap_find(
                block_bits(bl, bl->blocks[block_index]),
                (index % bl->block_size) + 1,
                true)) != -1) {
        return (block_index * bl->block_size) + i;
    }

find_next:;

    // try to advance to the next block
    dynlist_each(bl->blocks, it, block_index + 1) {
        if (!*it.el) { continue; }

        i = bitmap_find(block_bits(bl, *it.el), 0, true);

        // if i is -1, that means that this block *should* have been deallocated
        // but wasn't for some reason. if you hit this assert, something is
        // wrong in blklist_remove
        ASSERT(i != -1);

        return (ARR_PTR_INDEX(bl->blocks, it.el) * bl->block_size) + i;
    }

    // could not find next block, must be at the end
    return -1;
}

usize blklist_footprint(const blklist_t *bl) {
    usize n = 0;
    n += dynlist_footprint(bl->blocks);

    dynlist_each(bl->blocks, it) {
        if (*it.el) {
            n +=
                round_up_to_mult(
                    sizeof(bitmap_t) + BITMAP_SIZE_TO_BYTES(bl->block_size),
                    MAX_ALIGN)
                + (bl->t_size * bl->block_size);
        }
    }

    return n;
}

usize blklist_overhead(const blklist_t *bl) {
    usize n = 0;

    dynlist_each(bl->blocks, it) {
        if (!*it.el) { continue; }

        // add bitmap size
        n +=
            round_up_to_mult(
                sizeof(bitmap_t) + BITMAP_SIZE_TO_BYTES(bl->block_size),
                MAX_ALIGN);

        // add for number of unused entries
        n += bl->t_size * bitmap_count(block_bits(bl, *it.el), false);
    }

    return
        sizeof(blklist_t)
            + dynlist_size_bytes(bl->blocks)
            + n;
}

#endif // ifdef UTIL_IMPL
