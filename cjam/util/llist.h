#pragma once

#include "types.h"
#include "assert.h"
#include "macros.h"

// include in struct which forms an LLIST, fx:
// struct foo { LLIST_NODE(struct foo) node; }
// struct bar { LLIST(struct foo) foolist; }
#define LLIST_NODE(_T) struct { _T *next; }

// include in struct which tracks head of an LLIST, fx:
// struct foo { LLIST_NODE(struct foo) node; }
// struct bar { LLIST(struct foo) foolist; }
#define LLIST(_T) struct { _T *head; }

typedef void (*_llist_func_impl)(void*, void*, int);
void _llist_prepend_impl(void *_list, void *n, int offset);
void _llist_remove_impl(void *_list, void *n, int offset);

// init (clear) a LLIST field
#define llist_init(_plist) ({                                                \
        typeof(_plist) __p = (_plist);                                       \
        __p->head = NULL;                                                    \
    })

// init (clear) a LLIST_NODE field
#define llist_init_node(_pnode) ({                                           \
        typeof(_pnode) __p = (_pnode);                                       \
        __p->next = NULL;                                                    \
    })

// gets "parent" struct with llist field. fx:
// struct foo { LLIST_NODE(struct foo) node; }
// llist_parent(struct foo, list, ptr_to_foo->list) == ptr_to_foo
#define llist_parent(_type, _field, _ptr)                                    \
    ({                                                                       \
        typeof(_ptr) __ptr = (_ptr);                                         \
        __ptr ? ((_type*) (((u8*) (_ptr)) - offsetof(_type, _field))) : NULL;\
     })

#define _llist_each_impl(_field, _list, _it, _itname, _listname)             \
    typedef struct {                                                         \
        int i;                                                               \
        unconst(typeof((_list)->head)) el, _prev, _next;                     \
        bool _removed;                                                       \
    } _itname;                                                               \
    typeof(_list) _listname = (_list);                                       \
    for (_itname _it = (_itname) {                                           \
            0,                                                               \
            _listname->head,                                                 \
            NULL,                                                            \
            _listname->head ? _listname->head->_field.next : NULL,           \
            false                                                            \
        };                                                                   \
        _it.el;                                                              \
        _it.i++,                                                             \
        (_it._prev = _it._removed ? _it._prev : _it.el,                      \
         _it.el = _it._next,                                                 \
         _it._next = _it.el ? _it.el->_field.next: NULL,                     \
         _it._removed = false))

// iterate llist
// struct foo { LLIST_NODE(struct foo) node; }
// struct bar { LLIST(struct foo) foolist; }
// llist_each(node, &ptr_to_bar->foolist, it) { /* ... */ }
// NOTE: supports removal of current and previous elements, but not following
#define llist_each(_field, _plist, _it)                                      \
    _llist_each_impl(                                                        \
        _field,                                                              \
        _plist,                                                              \
        _it,                                                                 \
        CONCAT(_lli, __COUNTER__),                                           \
        CONCAT(_lll, __COUNTER__))

// remove item from llist under iteration
#define llist_remove_it(_field, _plist, _it) do {                            \
        typeof(_plist) __plist = (_plist);                                   \
        if (_it.el == __plist->head) { __plist->head = _it.el->_field.next; }\
        else { _it._prev->_field.next = _it.el->_field.next; }               \
        _it._removed = true;                                                 \
    } while (0)                                                              \

// prepend _n to list
#define llist_prepend(_field, _plist, _n) do {                               \
        CHECK_FIELD(typeof(*(_plist)), head);                                \
        ((_llist_func_impl)(_llist_prepend_impl))(                           \
            (_plist), (_n), offsetof(typeof(*(_n)), _field));                \
    } while (0)

// remove _n to list
#define llist_remove(_field, _plist, _n) do {                                \
        CHECK_FIELD(typeof(*(_plist)), head);                                \
        ((_llist_func_impl)(_llist_remove_impl))(                            \
            (_plist), (_n), offsetof(typeof(*(_n)), _field));                \
    } while (0)

// convert llist -> dynlist of pointers
#define llist_to_dynlist(_field, _plist) ({                                  \
        DYNLIST(typeof((_plist)->head)) _list = NULL;                        \
        llist_each(_field, _plist, _it) {                                    \
            *dynlist_push(_list) = _it.el;                                   \
        }                                                                    \
        _list;                                                               \
    })

// true if llist is empty
#define llist_empty(_plist) (!((_plist)->head))

#ifdef UTIL_IMPL
void _llist_prepend_impl(void *_list, void *n, int offset) {
    LLIST(void) *list = _list;
    LLIST_NODE(void) *fn = n + offset;

    ASSERT(!fn->next);

    if (list->head) {
        fn->next = list->head;
    }

    list->head = n;
}

void _llist_remove_impl(void *_list, void *n, int offset) {
    LLIST(void) *list = _list;
    LLIST_NODE(void) *fn = n + offset;

    // seek n from head
    void *node = list->head, *prev = NULL;
    while (node != n) {
        prev = node;
        node = ((LLIST_NODE(void)*) (node + offset))->next;
    }

    ASSERT(node);

    if (prev) {
        ((LLIST_NODE(void)*) (prev + offset))->next = fn->next;
    } else {
        // n is head
        list->head = fn->next;
    }

    fn->next = NULL;
}
#endif // ifdef UTIL_IMPL
