#pragma once

#include "types.h"

typedef int (*sort_cmp_f)(const void*, const void*, void*);

// cross platform reentrant sort
void sort(
    void *base_,
    usize n,
    usize width,
    sort_cmp_f cmp,
    void *arg);

#ifdef UTIL_IMPL

#include <stdlib.h>

typedef struct sort_data {
    void *arg;
    int (*cmp)(const void *a, const void *b, void*);
} sort_data_t;

int sort__argswap(void *arg, const void *a, const void *b) {
    sort_data_t *s = (sort_data_t*) arg;
    return s->cmp(a, b, s->arg);
}

void sort(
    void *base,
    usize n,
    usize width,
    sort_cmp_f cmp,
    void *arg) {
#if defined(TARGET_PLATFORM_macos)
    sort_data_t data = { .arg = arg, .cmp = cmp };
    qsort_r(base, n, width, &data, sort__argswap);
#elif defined(TARGET_PLATFORM_windows)
    sort_data_t data = { .arg = arg, .cmp = cmp };
    qsort_s(base, n, width, sort__argswap, &data);
#elif defined(TARGET_PLATFORM_linux) || defined(TARGET_PLATFORM_emscripten)
    // linux
    qsort_r(base, n, width, cmp, arg);
#else
    #error bad_platform
#endif // ifdef TARGET_PLATFORM_macos
}

#endif // ifdef UTIL_IMPL
