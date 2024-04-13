#pragma once

// assert _e, otherwise print _f with fmt __VA_ARGS__
#define _ASSERTX(_e, _expr, _fmt, ...)           \
    do {                                         \
        if (!(_e)) {                             \
            ERROR("(assertion expr %s)", _expr); \
            ERROR((_fmt), ##__VA_ARGS__);        \
            dumptrace(stderr);                   \
            exit(1);                             \
        }                                        \
    } while (0)

#define _ASSERT1(_e)      _ASSERTX(_e, #_e, "%s", "assertion failed")
#define _ASSERT2(_e, ...) _ASSERTX(_e, #_e, __VA_ARGS__)
#define _ASSERT3(_e, ...) _ASSERTX(_e, #_e, __VA_ARGS__)
#define _ASSERT4(_e, ...) _ASSERTX(_e, #_e, __VA_ARGS__)
#define _ASSERT5(_e, ...) _ASSERTX(_e, #_e, __VA_ARGS__)
#define _ASSERT6(_e, ...) _ASSERTX(_e, #_e, __VA_ARGS__)
#define _ASSERT7(_e, ...) _ASSERTX(_e, #_e, __VA_ARGS__)
#define _ASSERT8(_e, ...) _ASSERTX(_e, #_e, __VA_ARGS__)
#define ASSERT(...)       VMACRO(_ASSERT, __VA_ARGS__)

#include <stdio.h>
#include <stdlib.h>

// dumps a stacktrace to specified file
void dumptrace(FILE *f);

#include "macros.h" /* IWYU pragma: keep */
#include "log.h"    /* IWYU pragma: keep */

#ifdef UTIL_IMPL

#if defined(__GNUC__) && !defined(EMSCRIPTEN)
#include <unistd.h>
#include <execinfo.h>

void dumptrace(FILE *f) {
    void *callstack[128];
    const int frames = backtrace(callstack, 128);
    char **strs = backtrace_symbols(callstack, frames);

    for (int i = 0; i < frames; i++) {
        fprintf(f, "%s\n", strs[i]);
    }

    free(strs);
}
#else
void dumptrace(FILE *f) {
    fprintf(f, "(no stack trace on this platform)\n");
}
#endif // ifdef __GNUC__

#endif // ifdef UTIL_IMPL
