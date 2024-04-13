#pragma once

// #define CJAM_NO_MAIN to not have this header define a main function

#ifdef CJAM_IMPL

#undef UTIL_IMPL
#undef EXT_IMPL

// TODO: workaround, fix busted include order
#define UTIL_IMPL
#include "util/log.h" // IWYU pragma: keep
#include "util/assert.h" // IWYU pragma: keep
#undef UTIL_IMPL

#define RELOADHOST_CLIENT_IMPL
#include "reloadhost/reloadhost.h"
#undef RELOADHOST_CLIENT_IMPL

#define UTIL_IMPL
#include "util/util.h" // IWYU pragma: keep
#undef UTIL_IMPL

#define EXT_IMPL
#include "ext/ext.h" // IWYU pragma: keep
#undef EXT_IMPL

#endif // ifdef CJAM_IMPL

typedef struct {
    void (*init)();
    void (*deinit)();
    void (*frame)();
    void (*pre_reload)();
    void (*post_reload)();
} cjam_desc_t;

// request close
void cjam_quit();

#ifdef CJAM_IMPL

#include "util/hooks.h"

#ifdef EMSCRIPTEN
#include <emscripten.h>
#endif // ifdef EMSCRIPTEN

typedef struct {
    cjam_desc_t desc;
    bool quit;
} cjam_state_t;

static cjam_state_t _cj;
RELOAD_STATIC_GLOBAL(_cj);

void cjam_quit() {
    _cj.quit = true;
}

int RELOADHOST_ENTRY_NAME(
    int argc,
    char *argv[],
    reloadhost_op_e op,
    reloadhost_t *reloadhost);

#ifndef CJAM_NO_MAIN
int main(int argc, char *argv[]) {
    int res;
    if ((res = RELOADHOST_ENTRY_NAME(argc, argv, RELOADHOST_INIT, NULL))) {
        return res;
    }

#ifdef EMSCRIPTEN
    emscripten_set_main_loop(_cj.desc.frame, 0, 1);
#else 
    while (true) {
        if ((res = RELOADHOST_ENTRY_NAME(argc, argv, RELOADHOST_STEP, NULL))) {
            if (res == RELOADHOST_CLOSE_REQUESTED) {
                break;
            } else {
                return res;
            }
        }
    }
#endif // ifdef EMSCRIPTEN

    return RELOADHOST_ENTRY_NAME(argc, argv, RELOADHOST_DEINIT, NULL);
}
#endif // ifndef CJAM_NO_MAIN

// reloadhost entry point
int RELOADHOST_ENTRY_NAME(
    int argc,
    char *argv[],
    reloadhost_op_e op,
    reloadhost_t *reloadhost) {
    g_reloadhost = reloadhost;

    switch (op) {
    case RELOADHOST_INIT:;
        extern cjam_desc_t cjam_main();
        _cj.desc = cjam_main();
        if (_cj.desc.init) { _cj.desc.init(); }
        return 0;
    case RELOADHOST_DEINIT:
        if (_cj.desc.deinit) { _cj.desc.deinit(); }
        hook_call_hooks(HOOK_EXIT);
        return 0;
    case RELOADHOST_RELOAD:
        return 0;
    case RELOADHOST_PRE_RELOAD:
        hook_call_hooks(HOOK_PRE_RELOAD);
        if (_cj.desc.pre_reload) { _cj.desc.pre_reload(); }
        return 0;
    case RELOADHOST_POST_RELOAD:
        hook_call_hooks(HOOK_POST_RELOAD);
        if (_cj.desc.post_reload) { _cj.desc.post_reload(); }
        return 0;
    case RELOADHOST_STEP:
        if (_cj.desc.frame) { _cj.desc.frame(); }

        // manually reset thread-local allocator with 1 MiB max
        bump_allocator_reset(thread_scratch(), 1 * 1024 * 1024);

        return _cj.quit ? RELOADHOST_CLOSE_REQUESTED : 0;
    }

    return 0;
}

#endif // ifdef CJAM_IMPL
