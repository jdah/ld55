#pragma once

#include "types.h"

// xoshiro256+
typedef struct rand { u64 s[4]; } rand_t;

// NOTE: only works for sizeof(typeof(_min, _max)) <= sizeof(u64)
// random number between _min and _max
#define rand_n(_prand, _min, _max) ({ \
        const u64 _r = rand_n_u64((_prand)); \
        ((u64) (_max - _min) == U64_MAX) ? \
            ((_r)) \
            : (((typeof(_min)) ((_r) % (_max - _min + 1))) + _min); \
    })

// random (integer) sign (-1 or 1)
#define rand_sign(_prand) (rand_n((_prand), 0, 1) ? -1 : 1)

// create new rand with specified seed
rand_t rand_create(u64 seed);

// set seed _seed for rand_t *_prand
rand_t rand_seed(rand_t *r, u64 seed);

// random u64
u64 rand_n_u64(rand_t *r);

// random f64 between _min and _max
f64 rand_f64(rand_t *prand, f64 min, f64 max);

// random f32 between _min and _max
f32 rand_f32(rand_t *prand, f32 min, f32 max);

// random chance (0.0..1.0) returns true
f32 rand_chance(rand_t *prand, f32 p);

v2 rand_v2(rand_t *prand, v2 mi, v2 ma);

v3 rand_v3(rand_t *prand, v3 mi, v3 ma);

v4 rand_v4(rand_t *prand, v4 mi, v4 ma);

v3 rand_v3_dir(rand_t *prand);

v2 rand_v2_dir(rand_t *prand);

// random hemisphere-ical v3 to n
v3 rand_v3_hemisphere(rand_t *prand, v3 n);

// random v3 "conical" to n within th
v3 rand_v3_cone(rand_t *prand, v3 n, f32 theta);

// get v3 which is disc to n (perpendicular to direction)
v3 rand_v3_disc(rand_t *prand, v3 n);

// same as v3 variant, but with a v2
v2 rand_v2_cone(rand_t *prand, v2 n, f32 theta);

// get random v2 uniformly distributed in 2D triangle
v2 rand_v2_triangle(rand_t *prand, v2 a, v2 b, v2 c);

#ifdef UTIL_IMPL

#include "macros.h"
#include "math.h"

M_INLINE u64 rotl(const u64 x, int k) {
	return (x << k) | (x >> (64 - k));
}

// create new rand with specified seed
rand_t rand_create(u64 seed) {
    rand_t r = { 0 };
    rand_seed(&r, seed);
    return r;
}

// splitmix64
M_INLINE u64 rand_sm64(u64 u) {
    u64 q = (u + 0x9E3779B97f4A7C15);
    q = (q ^ (q >> 30)) * 0xBF58476D1CE4E5B9;
    q = (q ^ (q >> 27)) * 0x94D049BB133111EB;
    return q;
}

// set seed _seed for rand_t *_prand
rand_t rand_seed(rand_t *r, u64 seed) {
    u64 q = seed;
    r->s[0] = (q = rand_sm64(q));
    r->s[1] = (q = rand_sm64(q));
    r->s[2] = (q = rand_sm64(q));
    r->s[3] = (q = rand_sm64(q));
    return *r;
}

// random u64
u64 rand_n_u64(rand_t *r) {
    const u64
        q = rotl(r->s[0] + r->s[3], 23) + r->s[0],
        t = r->s[1] << 17;
    r->s[2] ^= r->s[0];
    r->s[3] ^= r->s[1];
    r->s[1] ^= r->s[2];
    r->s[0] ^= r->s[3];
    r->s[2] ^= t;
    r->s[3] = rotl(r->s[3], 45);
    return q;
}

// random f64 between _min and _max
f64 rand_f64(rand_t *prand, f64 min, f64 max) {
    const u64 r = rand_n_u64(prand);
    const union { u64 i; f64 d; } u =
        { .i = (((u64) 0x3FF) << 52) | (r >> 12) };
    return ((u.d - 1.0) * (max - min)) + min;
}

// random f64 between _min and _max
f32 rand_f32(rand_t *prand, f32 min, f32 max) {
    return (f32) rand_f64(prand, min, max);
}

// random chance (0.0..1.0) returns true
f32 rand_chance(rand_t *prand, f32 p) {
    return rand_f32(prand, 0.0f, 1.0f) <= p;
}

v2 rand_v2(rand_t *prand, v2 mi, v2 ma) {
    return
        v2_of(
            rand_f32(prand, mi.x, ma.x),
            rand_f32(prand, mi.y, ma.y));
}

v3 rand_v3(rand_t *prand, v3 mi, v3 ma) {
    return
        v3_of(
            rand_f32(prand, mi.x, ma.x),
            rand_f32(prand, mi.y, ma.y),
            rand_f32(prand, mi.z, ma.z));
}

v4 rand_v4(rand_t *prand, v4 mi, v4 ma) {
    return
        v4_of(
            rand_f32(prand, mi.x, ma.x),
            rand_f32(prand, mi.y, ma.y),
            rand_f32(prand, mi.z, ma.z),
            rand_f32(prand, mi.w, ma.w));
}

v3 rand_v3_dir(rand_t *prand) {
    return v3_normalize(rand_v3(prand, v3_of(-1.0f), v3_of(1.0f)));
}

v2 rand_v2_dir(rand_t *prand) {
    const f32 a = rand_f32(prand, 0.0f, TAU);
    return v2_of(cosf(a), sinf(a));
}

// random hemisphere-ical v3 to n
v3 rand_v3_hemisphere(rand_t *prand, v3 n) {
    v3 v = rand_v3(prand, v3_of(-1), v3_of(1));
    v = v3_normalize(v);
    v = v3_scale(v, sign(v3_dot(v, n)));
    return v;
}

// random v3 "conical" to n within th
// https://math.stackexchange.com/questions/56784
// NOTE: this is comically bad because I do not understand 3D math well enough
// to fix things. lookat??? there is a better way, I just cannot wrap my head
// around it.
v3 rand_v3_cone(rand_t *prand, v3 n, f32 theta) {
    if (theta < 0.000001f) { return n; }

    const f32 phi = rand_f32(prand, 0.0f, TAU);
    const f32 z = rand_f32(prand, cosf((theta)), 1.0f);
    const f32 q = sqrtf(1.0f - (z * z));
    const v3 v = v3_of(q * cosf(phi), q * sinf(phi), -z);

    n = v3_normalize(n);

    // if v3 is almost 0, 0, 1 offset ever so slightly so lookat doesn't bug
    v3 p = v3_of(0, 0, 1);
    if (fabsf(v3_dot(n, p) - 1.0f) < 0.00001f) {
        p = v3_normalize(v3_of(0, 0.00000001f, 0.999999f));
    }

    const m4 m = cam_lookat(v3_of(0), n, p);
    return v3_from(m4_mulv(m4_inv(m), v4_of(v, 0.0)));
}

v3 rand_v3_disc(rand_t *prand, v3 n) {
    // make sure vector isn't almost perpendicular
    v3 p = rand_v3_dir(prand);
    while (v3_distance(n, p) < 0.001f) {
        p = rand_v3_dir(prand);
    }

    return v3_normalize(v3_cross(n, p));
}

// same as above, but with a v2
v2 rand_v2_cone(rand_t *prand, v2 n, f32 theta) {
    const v3 v = rand_v3_cone(prand, v3_of(n, 0.0f), theta);
    return v2_from(v);
}

v2 rand_v2_triangle(rand_t *prand, v2 a, v2 b, v2 c) {
    // https://stackoverflow.com/questions/11178414
    const f32 r = rand_f32(prand, 0.0f, 1.0f), s = rand_f32(prand, 0.0f, 1.0f);

    // pick point between first two vertices
    const v2 d = v2_lerp(a, b, r);

    // weighted sample between point d between a and b, and c
    return v2_lerp(c, d, sqrtf(s));
}

#endif // ifdef UTIL_IMPL
