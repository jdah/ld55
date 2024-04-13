#pragma once

#include <stddef.h>

#ifdef RELOADHOST_CLIENT_ENABLED
#define RELOAD_VISIBLE
#else
#define RELOAD_VISIBLE static
#endif // ifdef RELOADHOST_CLIENT_ENABLED

// current reload host, NULL if there isn't one / not built with RELOADABLE
typedef struct reloadhost reloadhost_t;
extern reloadhost_t *g_reloadhost;

// must be defined by client application
#ifdef RELOADHOST_CLIENT_IMPL
reloadhost_t *g_reloadhost;
#endif // ifdef RELOADHOST_CLIENT_IMPL

typedef struct { void (*funcs[128])(); int n; } reloadhost_funclist_t;
extern reloadhost_funclist_t reloadhost_funclist;

// operations for f_rh_entry
// RELOADHOST_INIT: client should initialize
// RELOADHOST_DEINIT: client should close
// RELOADHOST_STEP: client step (loop operation)
// RELOADHOST_PRE_RELOAD: client is about to be reloaded
// RELOADHOST_RELOAD: client has been reloaded, but no function pointers or
//                    variables have been updated
// RELOADHOST_POST_RELOAD: client is about to be reloaded AND all pointers,
//                         variables, etc. have been reloaded
typedef enum {
    RELOADHOST_INIT,
    RELOADHOST_DEINIT,
    RELOADHOST_STEP,
    RELOADHOST_PRE_RELOAD,
    RELOADHOST_RELOAD,
    RELOADHOST_POST_RELOAD,
} reloadhost_op_e;

// f_rh_entry returns this to request that it be called with RELOADHOST_DEINIT
#define RELOADHOST_CLOSE_REQUESTED INT_MAX

// name of f_rh_entry function in client
#define RELOADHOST_ENTRY_NAME entry

typedef struct reloadhost reloadhost_t;

typedef int (*reloadhost_entry_f)(int, char *[], reloadhost_op_e, reloadhost_t*);

typedef struct reloadhost {
    // pointer to function pointer registry function
    //
    // usage:
    //
    // struct foo { int (*funcptr)(); }
    // int myfunc() { ... }
    // struct foo f = malloc(sizeof(struct foo));
    // f->funcptr = myfunc;
    // reloadhost->regfunc(&f->funcptr);
    //
    // now f->funcptr is properly changed on code reload
    void (*reg_fn)(void*);

    // see regfunc
    // when heap storage for function pointer is free'd, call delfunc() with the
    // same address to free the function pointer in the reloadhost
    void (*del_fn)(void*);

    // register a variable for reloading
    void (*update_var_fn)(const char*, void*, size_t);

    // unregister a variable for reloading
    void (*del_var_fn)(const char*);

    // loaded address of module
    void *addr;

    // userdata pointer, can be used by client for arbitrary storage on reload
    // host
    void *userdata;
} reloadhost_t;

/// FOR CLIENT APPLICATION ///

#if defined(RELOADHOST_CLIENT_ENABLED) && !defined(RELOADHOST)

#include "../util/range.h"

void reloadhost_static_var(
    const range_t *var,
    const char *name,
    const char *file,
    const char *func,
    const range_t *buf);

#define RELOAD_STATIC_GLOBAL_RANGE_IMPL(_range, _name, _ctor_name)             \
    static void _name(void) {                                                  \
        static thread_local char buf[1024];                                    \
        const range_t _rng = (_range);                                         \
        reloadhost_static_var(&_rng, #_range, __FILE__, "", RANGE_REF(buf));   \
    }                                                                          \
    __attribute__((constructor)) static void _ctor_name() {                    \
        reloadhost_funclist.funcs[reloadhost_funclist.n++] = _name;            \
    }

#define RELOAD_STATIC_GLOBAL_RANGE(_range) \
    RELOAD_STATIC_GLOBAL_RANGE_IMPL(\
        (_range), \
        CONCAT(_rgbr_, __COUNTER__), \
        CONCAT(_rgbr_ctor_, __COUNTER__))

#define RELOAD_STATIC_GLOBAL(_var) RELOAD_STATIC_GLOBAL_RANGE(RANGE((_var)))

#define RELOAD_STATIC_RANGE(_range) do {                                       \
        static thread_local char buf[1024];                                    \
        const range_t _rng = (_range);                                         \
        reloadhost_static_var(                                           \
            &_rng, #_range, __FILE__, __FUNCTION__, RANGE_REF(buf));    \
    } while (0)

#define RELOAD_STATIC(_ptr, _sz) _RELOAD_STATIC_IMPL((_ptr), _ptr, _sz)

#define RELOAD_STATIC_VAR(_var) RELOAD_STATIC_RANGE(RANGE((_var)))

#define RELOAD_FUNCPTR(_pfuncptr) do {                                         \
        if (g_reloadhost) { g_reloadhost->reg_fn((_pfuncptr)); }               \
     } while (0);

#else
#define RELOAD_STATIC_GLOBAL_RANGE(...)
#define RELOAD_STATIC_GLOBAL(...)
#define RELOAD_STATIC_RANGE(...)
#define RELOAD_STATIC(...)
#define RELOAD_STATIC_VAR(...)
#define RELOAD_FUNCPTR(...)
#endif // defined(RELOADHOST_CLIENT_ENABLED) && !defined(RELOADHOST)

#ifdef RELOADHOST_CLIENT_IMPL

#include <stdio.h>

#include "../util/range.h"
#include "../util/assert.h"

reloadhost_funclist_t reloadhost_funclist;

void reloadhost_static_var(
    const range_t *var,
    const char *name,
    const char *file,
    const char *func,
    const range_t *buf) {
    if (!g_reloadhost) { return; }

    char *bufstr = buf->ptr;
    if (!bufstr[0]) {
        int len = snprintf(bufstr, buf->size, "%s%s%s", file, func, name);
        ASSERT(len >= 0 && len < (int) buf->size, "%d", len);
    }

    g_reloadhost->update_var_fn(buf->ptr, var->ptr, var->size);
}

#endif // ifdef RELOADHOST_CLIENT_IMPL

/// END FOR CLIENT APPLICATION ///
