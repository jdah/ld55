#pragma once

#include "types.h"
#include "macros.h"
#include "sort.h"

// usage: FIXLIST(foo_t) foo_list;
#define FIXLIST(_T, _N) struct { _T arr[_N]; int n; }

// internal use only
// inserts into list at index, returns pointer to allocated space
void *_fixlist_insert_impl(void *data, int *n, int t_size, int cap, int index);

// internal use only
// removes from list at index
void _fixlist_remove_impl(void *data, int *n, int t_size, int cap, int index);

// TODO: doc
void _fixlist_sort_impl(void *data, int *n, int t_size, sort_cmp_f func, void *arg);

// TODO: doc
void *_fixlist_insert_sorted_impl(
    void *data, int *n, int t_size, int cap, sort_cmp_f func, void *arg, const void *val);

// TODO: doc
#define fixlist_index_of_ptr(_d, _p) (ARR_PTR_INDEX((_d).arr, (_p)))

// returns true if fixlist is full
#define fixlist_full(_d) ((_d).n >= (int) /* NOLINT */ ARRLEN((_d).arr))

// insert new element at _i, returns pointer to new element
#define fixlist_insert(_d, _i) ({                                                         \
        typeof(&(_d)) _p = &(_d);                                                         \
        (typeof(&_p->arr[0])) _fixlist_insert_impl(                                       \
            _p->arr, &_p->n, sizeof(_p->arr[0]), /* NOLINT */ ARRLEN(_p->arr), (_i));     \
     })

// prepend, returns pointer to new space
#define fixlist_prepend(_d) ({                                                            \
        typeof(&(_d)) _p = &(_d);                                                         \
        (typeof(&_p->arr[0])) _fixlist_insert_impl(                                       \
            _p->arr, &_p->n, sizeof(_p->arr[0]), /* NOLINT */ ARRLEN(_p->arr), 0);        \
     })

// append, returns pointer to new space
#define fixlist_append(_d) ({                                                             \
        typeof(&(_d)) _p = &(_d);                                                         \
        (typeof(&_p->arr[0])) _fixlist_insert_impl(                                       \
            _p->arr, &_p->n, sizeof(_p->arr[0]), /* NOLINT */ ARRLEN(_p->arr), _p->n);    \
     })

// remove from fixlist at index, returns element
#define fixlist_remove(_d, _i) ({                                                         \
        typeof(&(_d)) _p = &(_d);                                                         \
        typeof(_i) __i = (_i);                                                            \
        typeof(_p->arr[0]) _t = _p->arr[__i];                                             \
        _fixlist_remove_impl(                                                             \
            _p->arr, &_p->n, sizeof(_p->arr[0]), /* NOLINT */ ARRLEN(_p->arr), __i);      \
        _t;                                                                               \
     })

// pop from end of fixlist, returns element
#define fixlist_pop(_d) ({                                                                \
        typeof(&(_d)) _p = &(_d);                                                         \
        typeof(_p->arr[0]) _t = _p->arr[_p->n - 1];                                       \
        _fixlist_remove_impl(                                                             \
            _p->arr, &_p->n, sizeof(_p->arr[0]), /* NOLINT */ ARRLEN(_p->arr), _p->n - 1);\
        _t;                                                                               \
     })

// push, returns pointer to new space
#define fixlist_push fixlist_append

#define _fixlist_each_impl(_d, _it, _start, _end, _pname, _itname)                        \
    typedef struct {                                                                      \
        int i, _dl_end;                                                                   \
        typeof(&(_d).arr[0]) el; } _itname;                                               \
    typeof(&(_d)) _pname = &(_d);                                                         \
    for (_itname _it = ({                                                                 \
            typeof(_start) __start = (_start);                                            \
            (_itname) {                                                                   \
                .i = __start,                                                             \
                ._dl_end = (_end),                                                        \
                .el = &_pname->arr[__start],                                              \
            };                                                                            \
         });                                                                              \
         _it.i < _pname->n && _it.i < _it._dl_end;                                        \
         _it.i++, (_it.el = _it.i < _pname->n ? &_pname->arr[_it.i] : NULL))

#define _fixlist_each4(_l, _it, _start, _end)                                             \
    _fixlist_each_impl(                                                                   \
        _l,                                                                               \
        _it,                                                                              \
        _start,                                                                           \
        _end,                                                                             \
        CONCAT(_dlp, __COUNTER__),                                                        \
        CONCAT(_dli, __COUNTER__))

#define _fixlist_each3(_l, _it, _start)                                                   \
    _fixlist_each_impl(                                                                   \
        _l,                                                                               \
        _it,                                                                              \
        _start,                                                                           \
        INT_MAX,                                                                          \
        CONCAT(_dlp, __COUNTER__),                                                        \
        CONCAT(_dli, __COUNTER__))

#define _fixlist_each2(_l, _it)                                                           \
    _fixlist_each_impl(                                                                   \
        _l,                                                                               \
        _it,                                                                              \
        0,                                                                                \
        INT_MAX,                                                                          \
        CONCAT(_dlp, __COUNTER__),                                                        \
        CONCAT(_dli, __COUNTER__))

// usage: fixlist_each(<list>, <it>, [start?], [end?]) { ... }
// NOTE: list expansion is supported under iteration, but to remove the
// currently iterated element, fixlist_remove_it *must* be used.
#define fixlist_each(...) VMACRO(_fixlist_each, __VA_ARGS__)

// remove current iterated element under iteration
#define fixlist_remove_it(_d, _it) do {                                                   \
        fixlist_remove((_d), (_it).i);                                                    \
        (_it).i--;                                                                        \
    } while(0)

// copy one fixlist to another
#define fixlist_copy(_dst, _src) do {                                                     \
        STATIC_ASSERT(                                                                    \
            sizeof((_dst).arr[0]) == sizeof((_src).arr[0]) /* NOLINT */);                 \
        fixlist_each((_src), _it) { *fixlist_push((_dst)) = *_it.el; }                    \
    } while(0);

// sort a fixlist
#define fixlist_sort(_d, _sortfunc, _arg) do {                                            \
        typeof(&(_d)) _p = &(_d);                                                         \
        _fixlist_sort_impl(                                                               \
            _p->arr, &_p->n, sizeof(_p->arr[0]) /* NOLINT */,                             \
            (_sortfunc), (_arg));                                                         \
    } while (0)

// insert into a sorted fixlist, returns pointer to slot
#define fixlist_insert_sorted(_d, _sortfunc, _arg, _pval) ({                              \
        typeof(&(_d)) _p = &(_d);                                                         \
        (typeof(&_p->arr[0])) _fixlist_insert_sorted_impl(                                \
            _p->arr, &_p->n, sizeof(_p->arr[0]), /* NOLINT */                             \
            ARRLEN(_p->arr), _sortfunc, _arg, _pval);                                     \
     })

#ifdef UTIL_IMPL
#include "assert.h"

void *_fixlist_insert_impl(void *data, int *n, int t_size, int cap, int index) {
    if (*n + 1 > cap) {
        WARN("fixlist @ %p (cap %d) is out of space", data, cap);
        return NULL;
    }

    if (index != *n) {
        memmove(
            data + (t_size * (index + 1)),
            data + (t_size * (index + 0)),
            (*n - index) * t_size);
    }

    (*n)++;
    return data + (t_size * index);
}

void _fixlist_remove_impl(void *data, int *n, int t_size, int cap, int index) {
    ASSERT(*n != 0);
    ASSERT(index < *n);
    memmove(
        data + (index * t_size),
        data + ((index + 1) * t_size),
        (*n - index - 1) * t_size);
    (*n)--;
}

void _fixlist_sort_impl(void *data, int *n, int t_size, sort_cmp_f func, void *arg) {
    sort(
        data,
        *n,
        t_size,
        func,
        arg);
}

void *_fixlist_insert_sorted_impl(
    void *data, int *n, int t_size, int cap, sort_cmp_f func, void *arg, const void *val) {
    int low = 0, high = *n;
    while (low < high) {
        int mid = (low + high) / 2;
        if (func(data + (mid * t_size), val, arg) < 0) {
            low = mid + 1;
        } else {
            high = mid;
        }
    }

    return _fixlist_insert_impl(data, n, t_size, cap, low);
}
#endif // ifdef UTIL_IMPL
