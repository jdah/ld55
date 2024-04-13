#pragma once

#include "types.h"
#include "macros.h"
#include "assert.h"

// include in struct which forms a DLIST, fx:
// struct foo { DLIST_NODE(struct foo) node; }
// struct bar { DLIST(struct foo) foolist; }
#define DLIST_NODE(_T) struct { _T *prev, *next; }

// include in struct which tracks head/tail of a DLIST, fx:
// struct foo { DLIST_NODE(struct foo) node; }
// struct bar { DLIST(struct foo) foolist; }
#define DLIST(_T) struct { _T *head, *tail; }

void _dlist_prepend_impl(void *_list, void *n, int offset);
void _dlist_append_impl(void *_list, void *n, int offset);
void _dlist_remove_impl(void *_list, void *n, int offset);
void _dlist_insert_before_impl(void *_list, void *x, void *y, int offset);
void _dlist_insert_after_impl(void *_list, void *x, void *y, int offset);

// init (clear) a DLIST field
#define dlist_init(_plist) ({                                                \
        typeof(_plist) __p = (_plist);                                       \
        __p->head = NULL;                                                    \
        __p->tail = NULL;                                                    \
    })

// init (clear) a DLIST_NODE field
#define dlist_init_node(_pnode) ({                                           \
        typeof(_pnode) __p = (_pnode);                                       \
        __p->prev = NULL;                                                    \
        __p->next = NULL;                                                    \
    })

// gets "parent" struct with dlist field. fx:
// struct foo { DLIST_NODE(struct foo) node; }
// dlist_parent(struct foo, list, ptr_to_foo->list) == ptr_to_foo
#define dlist_parent(_type, _field, _ptr) ({                                 \
        typeof(_ptr) __ptr = (_ptr);                                         \
        __ptr ? ((_type*) (((u8*) (_ptr)) - offsetof(_type, _field))) : NULL;\
     })

// NOTE: the _itname struct included prev, next fields in order to support
// append/prepend/remove during iteration.
//
// removal is handled by checking if
// _it.prev->next is the same as _it.el, as if it isn't then _it.el was removed.
//
// we don't need to worry about a prepend as we can never iterate over something
// prepended under iteration.
//
// to support appending, we always hop to _it.el->next UNLESS something was
// removed.
//
// this does not, however, support the case that the last element is
// removed and then something is appended in the same iteration - the new
// element will NOT be iterated in that case.
#define _dlist_each_impl(_field, _plist, _it, _lname, _itname)               \
    CHECK_FIELD(typeof(*(_plist)), head);                                    \
    CHECK_FIELD(typeof(*(_plist)), tail);                                    \
    typeof(_plist) _lname = (_plist);                                        \
    typedef struct {                                                         \
        int i;                                                               \
        unconst(typeof((_plist)->head)) el, prev, next; } _itname;           \
    for (_itname _it =                                                       \
        (_itname) {                                                          \
            0,                                                               \
            _lname->head,                                                    \
            NULL,                                                            \
            _lname->head ? _lname->head->_field.next : NULL                  \
         };                                                                  \
         _it.el;                                                             \
         _it.i++,                                                            \
         (_it.el = (_it.prev && _it.prev->_field.next == _it.el) ?           \
            _it.el->_field.next                                              \
            : _it.next,                                                      \
          _it.prev = _it.el ? _it.el->_field.prev : NULL,                    \
          _it.next = _it.el ? _it.el->_field.next : NULL))

// iterate dlist
// struct foo { DLIST(struct foo) list; }
// struct bar { DLIST_HEAD(struct foo) foolist; }
// dlist_each(list, &ptr_to_bar->foolist, it) { /* ... */ }
#define dlist_each(_field, _phead, _it)                                      \
    _dlist_each_impl(                                                        \
        _field,                                                              \
        _phead,                                                              \
        _it,                                                                 \
        CONCAT(_dll, __COUNTER__),                                           \
        CONCAT(_dli, __COUNTER__))

typedef void (*_dlist_func_impl)(void*, void*, int);
typedef void (*_dlist_func2_impl)(void*, void*, void*, int);

// prepend _n to list
#define dlist_prepend(_field, _plist, _n) do {                               \
        CHECK_FIELD(typeof(*(_plist)), head);                                \
        CHECK_FIELD(typeof(*(_plist)), tail);                                \
        ((_dlist_func_impl)(_dlist_prepend_impl))(                           \
            (_plist), (_n), offsetof(typeof(*(_n)), _field));                \
    } while (0)

// append _n to list
#define dlist_append(_field, _plist, _n) do {                                \
        CHECK_FIELD(typeof(*(_plist)), head);                                \
        CHECK_FIELD(typeof(*(_plist)), tail);                                \
        ((_dlist_func_impl)(_dlist_append_impl))(                            \
            (_plist), (_n), offsetof(typeof(*(_n)), _field));                \
    } while (0);

// remove _n from list starting with _phead
#define dlist_remove(_field, _plist, _n) do {                                \
        CHECK_FIELD(typeof(*(_plist)), head);                                \
        CHECK_FIELD(typeof(*(_plist)), tail);                                \
        ((_dlist_func_impl)(_dlist_remove_impl))(                            \
            (_plist), (_n), offsetof(typeof(*(_n)), _field));                \
    } while (0);

// insert _y before _x
#define dlist_insert_before(_field, _plist, _x, _y) do {                     \
        CHECK_FIELD(typeof(*(_plist)), head);                                \
        CHECK_FIELD(typeof(*(_plist)), tail);                                \
        ((_dlist_func2_impl)(_dlist_insert_before_impl))(                    \
            (_plist), (_x), (_y), offsetof(typeof(*(_x)), _field));          \
    } while(0)                                                               \

// insert _y after _x
#define dlist_insert_after(_field, _plist, _x, _y) do {                     \
        CHECK_FIELD(typeof(*(_plist)), head);                                \
        CHECK_FIELD(typeof(*(_plist)), tail);                                \
        ((_dlist_func2_impl)(_dlist_insert_after_impl))(                    \
            (_plist), (_x), (_y), offsetof(typeof(*(_x)), _field));          \
    } while(0)

#ifdef UTIL_IMPL
void _dlist_append_impl(void *_list, void *n, int offset) {
    DLIST(void) *list = _list;
    DLIST_NODE(void) *fn = n + offset;

    ASSERT(!fn->prev);
    ASSERT(!fn->next);
    ASSERT(n != list->head);
    ASSERT(n != list->tail);

    if (list->tail) {
        DLIST_NODE(void) *ftail = list->tail + offset;
        ASSERT(!ftail->next);
        ftail->next = n;
    } else {
        ASSERT(!list->head);
        list->head = n;
    }

    fn->prev = list->tail;
    fn->next = NULL;
    list->tail = n;
}

void _dlist_prepend_impl(void *_list, void *n, int offset) {
    DLIST(void) *list = _list;
    DLIST_NODE(void) *fn = n + offset;

    ASSERT(!fn->prev);
    ASSERT(!fn->next);
    ASSERT(n != list->head);
    ASSERT(n != list->tail);

    if (list->head) {
        DLIST_NODE(void) *fhead = list->head + offset;
        ASSERT(!fhead->prev);
        fhead->prev = n;
    } else {
        ASSERT(!list->tail);
        list->tail = n;
    }

    fn->prev = NULL;
    fn->next = list->head;
    list->head = n;
}

void _dlist_remove_impl(void *_list, void *n, int offset) {
    DLIST(void) *list = _list;
    DLIST_NODE(void) *fn = n + offset;

    if (n == list->head) { list->head = fn->next; }
    if (n == list->tail) { list->tail = fn->prev; }

    if (fn->prev) { ((DLIST_NODE(void)*)(fn->prev + offset))->next = fn->next; }
    if (fn->next) { ((DLIST_NODE(void)*)(fn->next + offset))->prev = fn->prev; }

    fn->prev = NULL;
    fn->next = NULL;
}

void _dlist_insert_before_impl(
    void *_list, void *x, void *y, int offset) {
    DLIST(void) *list = _list;
    DLIST_NODE(void) *fx = x + offset, *fy = y + offset;

    if (!fx->prev) {
        _dlist_prepend_impl((void*) list, y, offset);
    } else {
        ((DLIST_NODE(void)*) (fx->prev + offset))->next = y;
        fy->prev = fx->prev;
        fy->next = x;
        fx->prev = y;
    }
}

void _dlist_insert_after_impl(
    void *_list, void *x, void *y, int offset) {
    DLIST(void) *list = _list;
    DLIST_NODE(void) *fx = x + offset, *fy = y + offset;

    if (!fx->next) {
        _dlist_append_impl((void*) list, y, offset);
    } else {
        ((DLIST_NODE(void)*) (fx->next + offset))->prev = y;
        fy->prev = x;
        fy->next = fx->next;
        fx->next = y;
    }
}
#endif // ifdef UTIL_IMPL
