#pragma once

#include "linalg.h"
#include "geo.h"

// 2D float box
typedef struct boxf {
    union {
        struct { v2 min, max; };
        v2 vs[2];
    };
} boxf_t;

// 2D int box
typedef struct boxi {
    union {
        struct { v2i min, max; };
        v2i vs[2];
    };
} boxi_t;

// 3D float box
typedef struct box3f {
    union {
        struct { v3 min, max; };
        v3 vs[2];
    };
} box3f_t;

#define boxf_mm(_mi, _ma) ((boxf_t) { .min = (_mi), .max = (_ma) })
#define boxi_mm(_mi, _ma) ((boxi_t) { .min = (_mi), .max = (_ma) })

#define boxf_m(_ma) ((boxf_t) { .min = v2_of(0), .max = (_ma) })
#define boxi_m(_ma) ((boxi_t) { .min = v2i_of(0), .max = (_ma) })

#define boxf_ps(_pos, _size) ({                                               \
        const v2 __pos = (_pos);                                               \
        ((boxf_t) { .min = __pos, .max = v2_add(__pos, (_size)) });           \
    })

#define boxi_ps(_pos, _size) ({                                                \
        const v2i __pos = (_pos);                                              \
        ((boxi_t) {                                                            \
         .min = __pos,                                                         \
         .max = v2i_add(__pos, v2i_sub((_size), v2i_of(1))) });                \
    })

#define boxf_ch(_center, _half) ({                                            \
        const v2 __c = (_center), __h = (_half);                               \
        ((boxf_t) { .min = v2_sub(__c, __h), .max = v2_add(__c, __h)});       \
    })

#define boxi_ch(_center, _half) ({                                             \
        const v2i __c = (_center), __h = (_half);                              \
        ((boxi_t) { .min = v2i_sub(__c, __h), .max = v2i_add(__c, __h)});      \
    })

#define PRIboxf "c%" PRIv2 " %" PRIv2 "]"
#define FMTboxf(_v) '[', FMTv2((_v).min), FMTv2((_v).max)

#define PRIboxi "c%" PRIv2i " %" PRIv2i "]"
#define FMTboxi(_v) '[', FMTv2i((_v).min), FMTv2i((_v).max)

// conversion functions
M_INLINE boxf_t boxf_from(boxi_t box) {
    return (boxf_t) {
        .min = v2_from_i(box.min),
        .max = v2_from_i(box.max),
    };
}


M_INLINE boxi_t boxi_from(boxf_t box) {
    return (boxi_t) {
        .min = v2i_from_v(box.min),
        .max = v2i_from_v(box.max),
    };
}

// returns true if box contains p
#define box_contains_impl(_name, _T, _t, _V, _Q, _U, _s)                      \
    M_INLINE bool _name(_T box, _V p) {                                       \
        return p.x >= box.min.x                                               \
            && p.x <= box.max.x                                               \
            && p.y >= box.min.y                                               \
            && p.y <= box.max.y;                                              \
    }

// returns true if a and b collide
#define box_collides_impl(_name, _T, _t, _V, _Q, _U, _s)                      \
    M_INLINE bool _name(_T a, _T b) {                                          \
        UNROLL(2)                                                              \
        for (usize i = 0; i < 2; i++) {                                        \
            if (a.min.raw[i] >= b.max.raw[i]                                   \
                || a.max.raw[i] <= b.min.raw[i]) {                             \
                return false;                                                  \
            }                                                                  \
        }                                                                      \
                                                                               \
        return true;                                                           \
    }

// returns overlap of a and b
#define box_intersect_impl(_name, _T, _t, _V, _Q, _U, _s)                     \
    M_INLINE _T _name(_T a, _T b) {                                            \
        _T r;                                                                  \
                                                                               \
        UNROLL(2)                                                              \
        for (usize i = 0; i < 2; i++) {                                        \
            r.min.raw[i] =                                                     \
                a.min.raw[i] < b.min.raw[i] ?                                  \
                    b.min.raw[i] : a.min.raw[i];                               \
            r.max.raw[i] =                                                     \
                a.max.raw[i] > b.max.raw[i] ?                                  \
                    b.max.raw[i] : a.max.raw[i];                               \
        }                                                                      \
                                                                               \
        return r;                                                              \
    }

// returns center of box
#define box_center_impl(_name, _T, _t, _V, _Q, _U, _s)                        \
    M_INLINE _V _name(_T a) {                                                  \
        return                                                                 \
            _Q##_add(                                                          \
                _Q##_divs(_Q##_sub(a.max, a.min), 2), a.min);                  \
    }

// translates box by v
#define box_translate_impl(_name, _T, _t, _V, _Q, _U, _s)                     \
    M_INLINE _T _name(_T a, _V v) {                                            \
        return (_T) { .min = _Q##_add(a.min, v), .max = _Q##_add(a.max, v) };  \
    }

// scales box by v, keeping center constant
#define box_scale_center_impl(_name, _T, _t, _V, _Q, _U, _s)                  \
    M_INLINE _T _name(_T a, _V v) {                                            \
        const _V                                                               \
            c = _t##_center(a),                                                \
            d = _Q##_sub(a.max, a.min),                                        \
            h = _Q##_divs(d, 2),                                               \
            e = _Q##_mul(h, v);                                                \
        return (_T) { .min = _Q##_sub(c, e), .max = _Q##_add(c, e) };          \
    }

// scales box by v, keeping min constant
#define box_scale_min_impl(_name, _T, _t, _V, _Q, _U, _s)                     \
    M_INLINE _T _name(_T a, _V v) {                                            \
        _V d = _Q##_sub(a.max, a.min);                                         \
        return (_T) { .min = a.min, .max = _Q##_add(a.min, _Q##_mul(d, v)) };  \
    }


// scales box by v, both min and max
#define box_scale_impl(_name, _T, _t, _V, _Q, _U, _s)                         \
    M_INLINE _T _name(_T a, _V v) {                                            \
        return (_T) { .min = _Q##_mul(a.min, v), .max = _Q##_mul(a.max, v) };  \
    }

// get points of box
#define box_points_impl(_name, _T, _t, _V, _Q, _U, _s)                        \
    M_INLINE void _name(_T a, _V ps[4]) {                                      \
        ps[0] = _U(a.min.x, a.min.y);                                          \
        ps[1] = _U(a.max.x, a.min.y);                                          \
        ps[2] = _U(a.max.x, a.max.y);                                          \
        ps[3] = _U(a.min.x, a.max.y);                                          \
    }

// get "half" (half size) of box
#define box_half_impl(_name, _T, _t, _V, _Q, _U, _s)                          \
    M_INLINE _V _name(_T a) {                                                  \
        return _Q##_sub(_t##_center(a), a.min);                                \
    }

// center box on new point
#define box_center_on_impl(_name, _T, _t, _V, _Q, _U, _s)                     \
    M_INLINE _T _name(_T a, _V c) {                                            \
        _V half = _t##_half(a);                                                \
        return (_T) { .min = _Q##_sub(c, half), .max = _Q##_add(c, half)};     \
    }

// center box in another box
#define box_center_in_impl(_name, _T, _t, _V, _Q, _U, _s)                     \
    M_INLINE _T _name(_T a, _T b) {                                            \
        return _t##_center_on(a, _t##_center(b));                              \
    }

// "sort" box min/max
#define box_sort_impl(_name, _T, _t, _V, _Q, _U, _s)                          \
    M_INLINE _T _name(_T a) {                                                  \
        return (_T) {                                                          \
            .min = _U(min(a.min.x, a.max.x), min(a.min.y, a.max.y)),           \
            .max = _U(max(a.min.x, a.max.x), max(a.min.y, a.max.y)),           \
        };                                                                     \
    }

// merge two AABBs
#define box_merge_impl(_name, _T, _t, _V, _Q, _U, _s)                         \
    M_INLINE _T _name(_T a, _T b) {                                            \
        return (_T) {                                                          \
            .min = _U(min(a.min.x, b.min.x), min(a.min.y, b.min.y)),           \
            .max = _U(max(a.max.x, b.max.x), max(a.max.y, b.max.y)),           \
        };                                                                     \
    }

#define box_line_impl(_name, _T, _t, _V, _Q, _U, _s)                          \
    M_INLINE bool _name(_T a, _V from, _V to) {                                \
        return _t##_contains(a, from)                                          \
            || _t##_contains(a, to)                                            \
            || intersect_seg_box(                                              \
                v2_of(from.x, from.y), v2_of(to.x, to.y),                      \
                v2_of(a.min.x, a.min.y), v2_of(a.max.x, a.max.y),              \
                NULL);                                                         \
    }                                                                          \

#define box_area_impl(_name, _T, _t, _V, _Q, _U, _s)                          \
    M_INLINE _s _name(_T a) {                                                  \
        const _V size = _t##_size(a);                                          \
        return size.x * size.y;                                                \
    }                                                                          \

// returns true if a contains b entirely
#define box_contains_other_impl(_name, _T, _t, _V, _Q, _U, _s)                \
    M_INLINE bool _name(_T a, _T b) {                                          \
        UNROLL(2)                                                              \
        for (usize i = 0; i < 2; i++) {                                        \
            if (b.min.raw[i] < a.min.raw[i]                                    \
                || b.max.raw[i] > a.max.raw[i]) {                              \
                return false;                                                  \
            }                                                                  \
        }                                                                      \
                                                                               \
        return true;                                                           \
    }

#define box_define_functions(_prefix, _T, _t, _V, _Q, _U, _S)                 \
    box_contains_impl(_prefix##_contains, _T, _t, _V, _Q, _U, _S)             \
    box_collides_impl(_prefix##_collides, _T, _t, _V, _Q, _U, _S)             \
    box_intersect_impl(_prefix##_intersect, _T, _t, _V, _Q, _U, _S)           \
    box_center_impl(_prefix##_center, _T, _t, _V, _Q, _U, _S)                 \
    box_translate_impl(_prefix##_translate, _T, _t, _V, _Q, _U, _S)           \
    box_scale_center_impl(_prefix##_scale_center, _T, _t, _V, _Q, _U, _S)     \
    box_scale_min_impl(_prefix##_scale_min, _T, _t, _V, _Q, _U, _S)           \
    box_scale_impl(_prefix##_scale, _T, _t, _V, _Q, _U, _S)                   \
    box_half_impl(_prefix##_half, _T, _t, _V, _Q, _U, _S)                     \
    box_center_on_impl(_prefix##_center_on, _T, _t, _V, _Q, _U, _S)           \
    box_center_in_impl(_prefix##_center_in, _T, _t, _V, _Q, _U, _S)           \
    box_points_impl(_prefix##_points, _T, _t, _V, _Q, _U, _S)                 \
    box_sort_impl(_prefix##_sort, _T, _t, _V, _Q, _U, _S)                     \
    box_merge_impl(_prefix##_merge, _T, _t, _V, _Q, _U, _S)                   \
    box_line_impl(_prefix##_vs_line, _T, _t, _V, _Q, _U, _S)                  \
    box_area_impl(_prefix##_area, _T, _t, _V, _Q, _U, _S)                     \
    box_contains_other_impl(_prefix##_contains_other, _T, _t, _V, _Q, _U, _S) \

// returns size of box
M_INLINE v2 boxf_size(boxf_t a) {
    return v2_sub(a.max, a.min);
}

// returns size of box
M_INLINE v2i boxi_size(boxi_t a) {
    return v2i_add(v2i_sub(a.max, a.min), v2i_of(1));
}

box_define_functions(boxf, boxf_t, boxf, v2, v2, v2_of, f32)
box_define_functions(boxi, boxi_t, boxi, v2i, v2i, v2i_of, i32)

// returns true if a intersect triangle t0, t1, t2
// triangle is assumed to be specified CCW
bool boxf_vs_triangle(boxf_t a, v2 t0, v2 t1, v2 t2);

#ifdef UTIL_IMPL

bool boxf_vs_triangle(boxf_t a, v2 t0, v2 t1, v2 t2) {
    const v2 points[3] = { t0, t1, t2 };

    // compute box of triangle
    boxf_t tri_box = { .min = t0, .max = t0 };
    for (int i = 1; i < 3; i++) {
        tri_box.min = v2_minv(tri_box.min, points[i]);
        tri_box.max = v2_maxv(tri_box.max, points[i]);
    }

    // rough check with box vs. box
    if (!boxf_collides(a, tri_box)) { return false; }

    // compute two triangle vs. triangle interesections with each triangle
    // from the AABB
    return
        intersect_triangle_triangle(
            a.min, v2_of(a.max.x, a.min.y), a.max,
            t0, t1, t2)
        || intersect_triangle_triangle(
            a.max, v2_of(a.min.x, a.max.y), a.min,
            t0, t1, t2);
}

#endif // ifdef UTIL_IMPL
