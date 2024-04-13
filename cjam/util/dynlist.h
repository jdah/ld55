#pragma once

#include "types.h"
#include "macros.h"
#include "assert.h"

// usage: DYNLIST(struct foo) foolist;
#define DYNLIST(_T) typeof((_T*)(NULL))

// minimal capacity
#define DYNLIST_MIN_CAP 1

// internal use only
// header of dynlist
typedef struct {
    int size, capacity, t_size;
    allocator_t *allocator;

#ifndef M_BITS_32
    void *_padding;
#endif // ifndef M_BITS_32

} dynlist_header_t;

STATIC_ASSERT(
    sizeof(dynlist_header_t) % MAX_ALIGN == 0,
    "dynlist_header_t has bad size");

// internal use only
// initialize a dynlist
void _dynlist_init_impl(
    void **plist, int t_size, int init_cap, allocator_t *allocator);

// internal use only
// destroy a dynlist
void _dynlist_destroy_impl(void **plist);

// internal use only
// resizes internal list buffer to accomodate (at least) new_cap
void _dynlist_realloc_impl(void **plist, int new_cap, bool allow_contract);

// internal use only
// inserts into list at index, returns pointer to allocated space
void *_dynlist_insert_impl(void **plist, int index);

// internal use only
// removes from list at index
void _dynlist_remove_impl(
    void **plist, int index, bool allow_contract);

// internal use only
// changes list size (not capacity) to n elements, ensuring space if necessary
void _dynlist_resize_impl(void **plist, int n, bool allow_contract);

// internal use only
// copies a dynlist
void *_dynlist_copy_impl(void **plist);

// internal use only
// copy psrc to pdst
void _dynlist_copy_from_impl(void **pdst, void **psrc);

// pointer to header of dynlist, must not be NULL
M_INLINE dynlist_header_t *dynlist_header(const void *l) {
    // TODO(refactor): ASSERT_DEBUG
    ASSERT(l, "dynlist is NULL");
    return (dynlist_header_t*) (((u8*) l) - sizeof(dynlist_header_t));
}

#define _dynlist_init3(_d, _allocator, _cap)                                  \
    _dynlist_init_impl(                                                       \
        (void**) &(_d), sizeof(*(_d)) /* NOLINT */, (_cap), (_allocator))

#define _dynlist_init2(_d, _allocator)                                        \
    _dynlist_init_impl(                                                       \
        (void**) &(_d), sizeof(*(_d)) /* NOLINT */, 4, (_allocator))

// init dynlist (d, _allocator, <cap?>)
#define dynlist_init(...) VMACRO(_dynlist_init, __VA_ARGS__)

#define _dynlist_create3(_T, _allocator, _cap) ({                             \
        DYNLIST(_T) list = NULL;                                              \
        _dynlist_init_impl(                                                   \
            (void**) &list, sizeof(_T) /* NOLINT */, (_cap), (_allocator));   \
        list;                                                                 \
    })

#define _dynlist_create2(_T, _allocator) _dynlist_create3(_T, _allocator, 4)

// create dynlist (_T, _allocator, <cap?>)
#define dynlist_create(...) VMACRO(_dynlist_create, __VA_ARGS__)

// destroy dynlist
#define dynlist_destroy(_d) ({                                                \
        typeof(_d) *_pd = &(_d);                                              \
        _dynlist_destroy_impl((void**) _pd);                                  \
        *_pd = NULL;                                                          \
    })

// returns size of dynlist
M_INLINE i32 dynlist_size(const void *l) {
    return dynlist_header(l)->size;
}

// returns size of dynlist in bytes
M_INLINE usize dynlist_size_bytes(const void *l) {
    const dynlist_header_t *h = dynlist_header(l);
    return h->size * h->t_size;
}

// returns size of dynlist in bytes
#define dynlist_size_bytes(_d)                                                \
    (dynlist_header(_d)->size * sizeof(*(_d) /* NOLINT */))

// returns size of dynlist + overhead in bytes
#define dynlist_footprint(_d)                                                \
    (sizeof(dynlist_header_t) + dynlist_size_bytes((_d)))

// returns internal allocated capacity of dynlist
M_INLINE i32 dynlist_capacity(const void *l) {
    return dynlist_header(l)->capacity;
}
// ensure capacity of at least _n elements
#define dynlist_ensure(_d, _n)                                                \
    _dynlist_realloc_impl((void**) &(_d), (_n), true)

// insert new element at _i, returns pointer to new element
#define dynlist_insert(_d, _i)                                                \
    (typeof(&(_d)[0])) _dynlist_insert_impl(                                  \
        (void**) &(_d), (_i))

// prepend, returns pointer to new space
#define dynlist_prepend(_d)                                                   \
    (typeof(&(_d)[0])) _dynlist_insert_impl(                                  \
        (void**) &(_d), 0)

// append, returns pointer to new space
#define dynlist_append(_d) ({                                                 \
        typeof(_d) *_pd = &(_d);                                              \
        dynlist_header_t *h = dynlist_header(*_pd);                           \
        (typeof(&(_d)[0])) _dynlist_insert_impl(                              \
            (void**) _pd, h ? h->size : 0);     \
    })

// remove from dynlist at index, returns element
#define dynlist_remove(_d, _i) ({                                             \
        typeof(_d) *_pd = &(_d);                                              \
        typeof(_i) __i = (_i);                                                \
        typeof((_d)[0]) _t = (*_pd)[__i];                                     \
        _dynlist_remove_impl(                                                 \
            (void**) _pd, __i, true); \
        _t;                                                                   \
    })

// remove from dynlist at index, returns element
#define dynlist_remove_no_realloc(_d, _i) ({                                  \
        typeof(_d) *_pd = &(_d);                                              \
        typeof(_i) __i = (_i);                                                \
        typeof((_d)[0]) _t = (*_pd)[__i];                                     \
        _dynlist_remove_impl(                                                 \
            (void**) _pd, __i, false);            \
        _t;                                                                   \
    })

// pop from dynlist, returns element
#define dynlist_pop(_d) ({                                                    \
        typeof(_d) *_pd = &(_d);                                              \
        dynlist_header_t *h = dynlist_header(*_pd);                           \
        typeof(*(_d)) _t = (*_pd)[h->size - 1];                               \
        _dynlist_remove_impl(                                                 \
            (void**) _pd, h->size - 1, true);     \
        _t;                                                                   \
    })

// push, returns pointer to new space
#define dynlist_push dynlist_append

// sets dynlist size
#define dynlist_resize(_d, _n)                                                \
    _dynlist_resize_impl(                                                     \
        (void**) &(_d), _n, true)

// sets dynlist size but doesn't contract it
#define dynlist_resize_no_contract(_d, _n)                                    \
    _dynlist_resize_impl(                                                     \
        (void**) &(_d), _n, false)

// resize to zero
#define dynlist_clear(_d) dynlist_resize((_d), 0)

// append all of one dynlist to another
#define dynlist_push_all(_d, _e) ({                                            \
        typeof(_d) *_pd = &(_d);                                               \
        typeof(_e) *_pe = &(_e);                                               \
        ASSERT(dynlist_header(*_pd)->t_size == dynlist_header(*_pe)->t_size);  \
        const int _offset = dynlist_size(*_pd), _n = dynlist_size(*_pe);       \
        dynlist_resize(*_pd, _offset + dynlist_size(*_pe));                    \
        for (int _i = 0; _i < _n; _i++) { (*_pd)[_offset + _i] = (*_pe)[_i]; } \
    })

#define _dynlist_each_impl(_d, _it, _start, _end, _pname, _itname)            \
    typedef struct {                                                          \
        int i, _dl_end;                                                       \
        unconst(typeof(&(_d)[0])) el; } _itname;                              \
    typeof(&(_d)) _pname = &(_d);                                             \
    for (_itname _it = ({                                                     \
            typeof(_start) __start = (_start);                                \
            (_itname) {                                                       \
                .i = __start,                                                 \
                ._dl_end = (_end),                                            \
                .el = (*_pname) ? &(*_pname)[__start] : NULL                  \
            };                                                                \
         });                                                                  \
         (*_pname)                                                            \
            && _it.i < dynlist_size(*_pname)                                  \
            && _it.i < _it._dl_end;                                           \
         _it.i++,                                                             \
         (_it.el = (*_pname) ? &(*_pname)[_it.i] : NULL))

#define _dynlist_each4(_l, _it, _start, _end)                                 \
    _dynlist_each_impl(                                                       \
        _l,                                                                   \
        _it,                                                                  \
        _start,                                                               \
        _end,                                                                 \
        CONCAT(_dlp, __COUNTER__),                                            \
        CONCAT(_dli, __COUNTER__))

#define _dynlist_each3(_l, _it, _start)                                       \
    _dynlist_each_impl(                                                       \
        _l,                                                                   \
        _it,                                                                  \
        _start,                                                               \
        INT_MAX,                                                              \
        CONCAT(_dlp, __COUNTER__),                                            \
        CONCAT(_dli, __COUNTER__))

#define _dynlist_each2(_l, _it)                                               \
    _dynlist_each_impl(                                                       \
        _l,                                                                   \
        _it,                                                                  \
        0,                                                                    \
        INT_MAX,                                                              \
        CONCAT(_dlp, __COUNTER__),                                            \
        CONCAT(_dli, __COUNTER__))

// usage: dynlist_each(<list>, <it>, [start?], [end?]) { ... }
// NOTE: list expansion is supported under iteration, but to remove the
// currently iterated element, dynlist_remove_it *must* be used.
#define dynlist_each(...) VMACRO(_dynlist_each, __VA_ARGS__)

// remove current iterated element under iteration
// TODO(refactor): investigate bug ????
// TODO: may have a bug, things get messed up if removing first element and
// appending to list immediately following
#define dynlist_remove_it(_d, _it) do {                                       \
        typeof(&(_d)) __pd = &(_d);                                           \
        typeof((_d)[0]) _x = dynlist_remove(*__pd, (_it).i);                  \
        (_it).i--;                                                            \
        _x;                                                                   \
    } while(0)

// create copy of a dynlist
#define dynlist_copy(_d)                                                      \
    ((typeof(_d))                                                             \
        _dynlist_copy_impl((void**) &(_d)))

// copy dynlist into another dynlist, returns _dst
#define dynlist_copy_from(_dst, _src) ({                                      \
        CHECK_TYPE(typeof(*(_dst)), *(_src));                                 \
        _dynlist_copy_from_impl(                                              \
            (void**) &(_dst),                                                 \
            (void**) &(_src));                                       \
    })

// sort a dynlist
#define dynlist_sort(_d, _sortfunc, _arg) do {                              \
        _dynlist_sort_impl((void**) &(_d), (_sortfunc), (_arg));            \
    } while (0)

// insert into a sorted dynlist, returns pointer to slot
#define dynlist_insert_sorted(_d, _sortfunc, _arg, _pval)                   \
    ((typeof(_d))                                                           \
        _dynlist_insert_sorted_impl(                                        \
            (void**) &(_d), (_sortfunc), (_arg), (_pval)))

#ifdef UTIL_IMPL

#include "alloc.h"
#include "sort.h"

void _dynlist_init_impl(
    void **plist, int t_size, int init_cap, allocator_t *allocator) {
    ASSERT(!*plist);

    init_cap = max(init_cap, DYNLIST_MIN_CAP);

    dynlist_header_t *h =
        mem_alloc(
            allocator,
            sizeof(dynlist_header_t) + (t_size * init_cap));
    ASSERT(h);

    *h = (dynlist_header_t) {
        .size = 0,
        .capacity = init_cap,
        .t_size = t_size,
        .allocator = allocator,
    };
    *plist = h + 1;
}

void _dynlist_destroy_impl(void **plist) {
    dynlist_header_t *h = dynlist_header(*plist);
    mem_free(h->allocator, h);
    *plist = NULL;
}

void _dynlist_realloc_impl(void **plist, int new_cap, bool allow_contract) {
    ASSERT(new_cap >= 0);

    dynlist_header_t *h = dynlist_header(*plist);

    if (new_cap == h->capacity) {
        // nothing to do
        return;
    }

    if (new_cap == 0) {
        // revert to minimal size if necessary
        if (h->capacity > DYNLIST_MIN_CAP) {
            dynlist_header_t *new_header =
                mem_alloc(
                    h->allocator,
                    sizeof(dynlist_header_t) + (h->t_size * DYNLIST_MIN_CAP));
            ASSERT(new_header);

            *new_header = (dynlist_header_t) {
                .size = h->size,
                .capacity = DYNLIST_MIN_CAP,
                .allocator = h->allocator,
                .t_size = h->t_size,
            };

            mem_free(h->allocator, h);
            *plist = new_header + 1;
        }

        return;
    }

    // never allow 0 capacity from this point
    int capacity = max(h->capacity, DYNLIST_MIN_CAP);

    // TODO: loops bad
    while (new_cap > capacity) {
        capacity *= 2;
    }

    if (allow_contract) {
        while (new_cap <= (capacity / 2)) {
            capacity /= 2;
        }
    }

    if (h->capacity != capacity) {
        dynlist_header_t *new_header =
            mem_alloc(
                h->allocator,
                sizeof(dynlist_header_t) + (h->t_size * capacity));

        *new_header = (dynlist_header_t) {
            .size = h->size,
            .capacity = capacity,
            .t_size = h->t_size,
            .allocator = h->allocator,
        };

        memcpy(new_header + 1, h + 1, h->t_size * min(capacity, h->size));
        mem_free(h->allocator, h);
        *plist = new_header + 1;
    }
}

void *_dynlist_insert_impl(void **plist, int index) {
    ASSERT(plist);
    ASSERT(*plist);

    _dynlist_realloc_impl(
        plist,
        dynlist_header(*plist)->size + 1,
        false);

    dynlist_header_t *h = dynlist_header(*plist);
    void *data = h + 1;

    if (index != h->size) {
        memmove(
            data + (h->t_size * (index + 1)),
            data + (h->t_size * (index + 0)),
            (h->size - index) * h->t_size);
    }

    h->size++;
    return data + (h->t_size * index);
}

void _dynlist_remove_impl(
    void **plist, int index, bool allow_contract) {
    ASSERT(plist);
    ASSERT(*plist);

    dynlist_header_t *h = dynlist_header(*plist);

    ASSERT(h->size != 0);
    ASSERT(index < h->size);

    void *data = h + 1;
    memmove(
        data + (index * h->t_size),
        data + ((index + 1) * h->t_size),
        (h->size - index - 1) * h->t_size);

    _dynlist_realloc_impl(plist, h->size - 1, allow_contract);

    dynlist_header(*plist)->size--;
}

void _dynlist_resize_impl(void **plist, int n, bool allow_contract) {
    ASSERT(plist);
    ASSERT(*plist);

    if (dynlist_header(*plist)->capacity < n) {
        _dynlist_realloc_impl(plist, max(n, DYNLIST_MIN_CAP), allow_contract);
    }

    dynlist_header(*plist)->size = n;
}


void *_dynlist_copy_impl(void **plist) {
    ASSERT(plist);
    ASSERT(*plist);

    dynlist_header_t *h = dynlist_header(*plist);

    DYNLIST(void) new_list = NULL;
    _dynlist_init_impl(&new_list, h->t_size, h->size, h->allocator);

    memcpy(new_list, *plist, h->t_size * h->size);

    return new_list;
}

void _dynlist_copy_from_impl(void **pdst, void **psrc) {
    dynlist_header_t *h = dynlist_header(*psrc);
    _dynlist_resize_impl(pdst, h->size, true);
    memcpy(*pdst, *psrc, h->size * h->t_size);
}


void _dynlist_sort_impl(void **plist, sort_cmp_f func, void *arg) {
    dynlist_header_t *h = dynlist_header(*plist);
    sort(
        *plist,
        h->size,
        h->t_size,
        func,
        arg);
}

void *_dynlist_insert_sorted_impl(
    void **plist, sort_cmp_f func, void *arg, const void *val) {
    dynlist_header_t *h = dynlist_header(*plist);
    int low = 0, high = h->size;
    while (low < high) {
        int mid = (low + high) / 2;
        if (func((*plist) + (mid * h->t_size), val, arg) < 0) {
            low = mid + 1;
        } else {
            high = mid;
        }
    }

    return _dynlist_insert_impl(plist, low);
}
#endif // ifdef UTIL_IMPL
