#pragma once

#include "../macros.h"
#include "../types.h"

#define PI 3.14159265359f
#define TAU (2.0f * PI)
#define PI_2 (PI / 2.0f)
#define PI_3 (PI / 3.0f)
#define PI_4 (PI / 4.0f)
#define PI_6 (PI / 6.0f)
#define PI_8 (PI / 8.0f)
#define PI_10 (PI / 10.0f)
#define PI_12 (PI / 12.0f)
#define PI_16 (PI / 16.0f)

#define rads_from_degs(_d) ((_d) * (PI / 180.0f))
#define degs_from_rads(_d) ((_d) * (180.0f / PI))

#define min(_a, _b) ({                                                         \
        typeof((_a) + (_b)) __a = (_a), __b = (_b);                            \
        __a < __b ? __a : __b;                                                 \
    })

#define max(_a, _b) ({                                                         \
        typeof((_a) + (_b)) __a = (_a), __b = (_b);                            \
        __a > __b ? __a : __b;                                                 \
    })

// clamp _x such that _x is in [_mi.._ma]
#define clamp(_x, _mi, _ma) (min(max(_x, _mi), _ma))

// clamp _x such that its magnitude does not exceed _v
#define clamp_mag(_x, _v)                                                      \
    ({ typeof(_v) __v = fabsf((_v)); clamp((_x), -__v, __v); })

// -1, 0, 1 depending on _f < 0, _f == 0, _f > 0
#define sign(_f) ({                                                            \
        typeof(_f) __f = (_f);                                                 \
        (__f > 0) - (__f < 0);                                                 \
    })

// clamp(x, 0, 1)
#define saturate(_x) clamp((_x), 0.0f, 1.0f)

// round _n to nearest multiple of _mult
#define round_up_to_mult(_n, _mult) ({                                         \
        typeof(_mult) __mult = (_mult);                                        \
        typeof(_n) _m = (_n) + (__mult - 1);                                   \
        _m - (_m % __mult);                                                    \
    })

// round _n to nearest multiple of _mult
#define round_up_to_multf(_n, _mult) ({                                        \
        typeof(_mult) __mult = (_mult);                                        \
        typeof(_n) _m = (_n) + (__mult - 1);                                   \
        _m - fmodf(_m, __mult);                                                \
    })

// round float to nearest power of two
#define round_to_potf(_f) ({                                                   \
        typeof(_f) _g = (_f);                                                  \
        sign(_g) * powf(2.0f, roundf(log2f(fabsf(_g))));                       \
    })

// round integer up to nearest power of two
#define round_up_to_pot(_i) ({                                                 \
        typeof(_i) _x = (_i);                                                  \
        1 << ((sizeof(_x) * 8) - __builtin_clz(_x - 1));                       \
    })

// returns fractional part of float _f
#define fract(_f) ({ typeof(_f) __f = (_f); __f - ((i64) (__f)); })

// if _x is nan, returns _alt otherwise returns _x
#define ifnan(_x, _alt) ({ typeof(_x) __x = (_x); isnan(__x) ? (_alt) : __x; })

// give alternative values for x if it is nan or inf
#define ifnaninf(_x, _nan, _inf) ({                                            \
        typeof(_x) __x = (_x);                                                 \
        isnan(__x) ? (_nan) : (isinf(__x) ? (_inf) : __x);                     \
    })

// lerp from _a -> _b by _t
#define lerp(_a, _b, _t) ({                                                    \
        typeof(_t) __t = (_t);                                                 \
        (typeof(_a)) (((_a) * (1 - __t)) + ((_b) * __t));                      \
    })

// compute a lerp term t with some factor _f and delta time _dt (seconds)
#define dtlerp_t(_f, _dt) (1.0f - expf(-(_f) * (_dt)))

// lerp from _a -> _b with delta time for some factor in 0..1 _f and delta time
#define dtlerp(_a, _b, _f, _dt) lerp((_a), (_b), dtlerp_t((_f), (_dt)))

// number of 1 bits in number
#define popcount(_x) __builtin_popcountll((_x))

// number of 0 bits in (unsigned) number
#define invpopcount(_x) (_Generic((_x),                                        \
    u8: (64 - __builtin_popcountll(0xFFFFFFFFFFFFFF00 | (u64) (_x))),          \
    u16: (64 - __builtin_popcountll(0xFFFFFFFFFFFF0000 | (u64) (_x))),         \
    u32: (64 - __builtin_popcountll(0xFFFFFFFF00000000 | (u64) (_x))),         \
    u64: (64 - __builtin_popcountll( (u64) (_x)))))

// (C)ount (L)eading (Z)eros
#define clz(_x) (__builtin_clz((_x)))

// (C)ount (T)railing (Z)eros
#define ctz(_x) (__builtin_ctz((_x)))

// true if a_ and b_ are within eps_ of each other
#define feq_eps(a_, b_, eps_) (fabs((a_) - (b_)) <= (eps_))

// convert int to float (by bits)
// corresponds to GLSL intBitsToFloat
M_INLINE f32 i32_bits_to_f32(i32 x) {
    union { f32 f; i32 i; } u;
    u.i = x;
    return u.f;
}

// convert float to int (by bits)
// corresponds to GLSL floatBitsToInt
M_INLINE int f32_bits_to_i32(i32 x) {
    union { f32 f; i32 i; } u;
    u.f = x;
    return u.i;
}
