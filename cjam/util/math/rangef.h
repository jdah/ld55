#pragma once

#include "../types.h"

typedef union rangef {
    struct { f32 z0, z1; };
    struct { f32 min, max; };
    struct { f32 lo, hi; };
    f32 zs[2];
} rangef_t;

// lerp in rangef by t
#define rangef_lerp(_zr, _t) ({                                                \
        const rangef_t __zr = (_zr);                                           \
        lerp(__zr.z0, __zr.z1, (_t));                                          \
    })

// clamp to be within rangef
#define rangef_clamp(_zr, _x) ({                                               \
        const rangef_t __zr = (_zr);                                           \
        clamp((_x), __zr.z0, __zr.z1);                                         \
    })
