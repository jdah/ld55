#pragma once

#include "macros.h"
#include "types.h"

#define NS_TO_SECS(_ns) ((_ns) / 1000000000.0)
#define SECS_TO_NS(_s) ((_s) * 1000000000.0)

// time since start in nanoseconds
u64 time_ns();

// time since start in seconds
M_INLINE f64 time_s() {
    return time_ns() / 1000000000.0;
}

#ifdef UTIL_IMPL

#include <SDL2/SDL.h>
#include "../reloadhost/reloadhost.h"

static u64 time_start, time_freq;

RELOAD_STATIC_GLOBAL(time_start)
RELOAD_STATIC_GLOBAL(time_freq)

u64 time_ns() {
    const u64 now = SDL_GetPerformanceCounter();

    if (!time_start) {
        time_start = now;
    }

    if (!time_freq) {
        time_freq = SDL_GetPerformanceFrequency();
    }

    const u64 diff = now - time_start;
    return (diff / (f64) time_freq) * 1000000000.0;
}

#endif // ifdef UTIL_IMPL
