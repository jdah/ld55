#pragma once

#include "../../lib/cglm/include/cglm/types-struct.h"    /* IWYU pragma: export */
#include "../../lib/cglm/include/cglm/struct/vec2.h"     /* IWYU pragma: export */
#include "../../lib/cglm/include/cglm/struct/ivec2.h"    /* IWYU pragma: export */
#include "../../lib/cglm/include/cglm/struct/vec3.h"     /* IWYU pragma: export */
#include "../../lib/cglm/include/cglm/struct/ivec3.h"    /* IWYU pragma: export */
#include "../../lib/cglm/include/cglm/struct/vec4.h"     /* IWYU pragma: export */
#include "../../lib/cglm/include/cglm/struct/mat2.h"     /* IWYU pragma: export */
#include "../../lib/cglm/include/cglm/struct/mat3.h"     /* IWYU pragma: export */
#include "../../lib/cglm/include/cglm/struct/mat4.h"     /* IWYU pragma: export */
#include "../../lib/cglm/include/cglm/struct/affine.h"   /* IWYU pragma: export */
#include "../../lib/cglm/include/cglm/struct/frustum.h"  /* IWYU pragma: export */
#include "../../lib/cglm/include/cglm/struct/plane.h"    /* IWYU pragma: export */
#include "../../lib/cglm/include/cglm/struct/box.h"      /* IWYU pragma: export */
#include "../../lib/cglm/include/cglm/struct/color.h"    /* IWYU pragma: export */
#include "../../lib/cglm/include/cglm/struct/io.h"       /* IWYU pragma: export */
#include "../../lib/cglm/include/cglm/struct/cam.h"      /* IWYU pragma: export */
#include "../../lib/cglm/include/cglm/struct/quat.h"     /* IWYU pragma: export */
#include "../../lib/cglm/include/cglm/struct/euler.h"    /* IWYU pragma: export */
#include "../../lib/cglm/include/cglm/struct/project.h"  /* IWYU pragma: export */
#include "../../lib/cglm/include/cglm/struct/sphere.h"   /* IWYU pragma: export */
#include "../../lib/cglm/include/cglm/struct/curve.h"    /* IWYU pragma: export */
#include "../../lib/cglm/include/cglm/struct/affine2d.h" /* IWYU pragma: export */
#include "../../lib/cglm/include/cglm/cglm.h"            /* IWYU pragma: export */

#include "../macros.h"
#include "../types.h"           /* IWYU pragma: export */

#include "util.h"       /* IWYU pragma: export */
#include "aliases.h"    /* IWYU pragma: export */

#define MKV2I2(_x, _y) ((v2i) {{ (_x), (_y) }})
#define MKV2I1(_s) ({ const int __s = (_s); ((v2i) {{ __s, __s }}); })
#define MKV2I0() ((v2i) {{ 0, 0 }})
#define v2i_of(...) (VMACRO(MKV2I, __VA_ARGS__))

#define MKV3I3(_x, _y, _z) ((v3i) {{ (_x), (_y), (_z) }})
#define MKV3I1(_s) ({ const int __s = (_s); ((v3i) {{ __s, __s, __s }}); })
#define MKV3I0() ((v3i) {{ 0, 0 }})
#define v3i_of(...) VMACRO(MKV3I, __VA_ARGS__)

#define MKV4I4(_x, _y, _z, _w) ((v4i) {{ (_x), (_y), (_z), (_w) }})
#define MKV4I3(_v2, _z, _w) ({ const v2i _v = (_v2); ((v4i) {{ _v.x, _v.y, (_z), (_w) }}); })
#define MKV4I2(_v3, _w) ({ const v3i _v = (_v3); ((v4i) {{ _v.x, _v.y, _v.z, (_w) }}); })
#define MKV4I1(_s) ({ const int __s = (_s); ((v4i) {{ __s, __s, __s, __s }}); })
#define MKV4I0() ((v4i) {{ 0, 0 }})
#define v4i_of(...) VMACRO(MKV4I, __VA_ARGS__)

#define MKV22(_x, _y) ((v2) {{ (_x), (_y) }})
#define MKV21(_s) ({ const f32 __s = (_s); ((v2) {{ __s, __s }}); })
#define MKV20() ((v2) {{ 0, 0 }})
#define v2_of(...) VMACRO(MKV2, __VA_ARGS__)

#define MKV2_CONST2(_x, _y) (((v2) {{ (_x), (_y) }}))
#define MKV2_CONST1(_s) ((v2) {{ _s, _s }})
#define MKV2_CONST0() ((v2) {{ 0, 0 }})
#define v2_const(...) VMACRO(MKV2_CONST, __VA_ARGS__)

#define MKV33(_x, _y, _z) ((v3) {{ (_x), (_y), (_z) }})
#define MKV32(_v2, _z) ({ const v2 _v = (_v2); ((v3) {{ _v.x, _v.y, (_z) }}); })
#define MKV31(_s) ({ const f32 __s = (_s); ((v3) {{ __s, __s, __s }}); })
#define MKV30() ((v3) {{ 0, 0 }})
#define v3_of(...) VMACRO(MKV3, __VA_ARGS__)

#define MKV3_CONST3(_x, _y, _z) ((v3) {{ (_x), (_y), (_z) }})
#define MKV3_CONST2(_v2, _z) (((v3) {{ _v2.x, _v2.y, (_z) }}))
#define MKV3_CONST1(_s) ((v3) {{ _s, _s, _s }})
#define MKV3_CONST0() ((v3) {{ 0, 0 }})
#define v3_const(...) VMACRO(MKV3_CONST, __VA_ARGS__)

#define MKV44(_x, _y, _z, _w) ((v4) {{ (_x), (_y), (_z), (_w) }})
#define MKV43(_v2, _z, _w) ({ const v2 _v = (_v2); ((v4) {{ _v.x, _v.y, (_z), (_w) }}); })
#define MKV42(_v3, _w) ({ const v3 _v = (_v3); ((v4) {{ _v.x, _v.y, _v.z, (_w) }}); })
#define MKV41(_s) ({ const f32 __s = (_s); ((v4) {{ __s, __s, __s, __s }}); })
#define MKV40() ((v4) {{ 0, 0 }})
#define v4_of(...) VMACRO(MKV4, __VA_ARGS__)

#define MKV4_CONST4(_x, _y, _z, _w) ((v4) {{ (_x), (_y), (_z), (_w) }})
#define MKV4_CONST2(_v3, _w) (((v4) {{ (_v3).x, (_v3).y, (_v3).z, (_w), }}))
#define MKV4_CONST1(_s) ((v4) {{ _s, _s, _s, _s }})
#define MKV4_CONST0() ((v4) {{ 0, 0, 0, 0 }})
#define v4_const(...) VMACRO(MKV4_CONST, __VA_ARGS__)

// use like PRIu**, fx.
// printf("%" PRIv2, FMTv2(vector));
#define PRIv2 "c%.3f, %.3f)"
#define PRIv3 "c%.3f, %.3f, %.3f)"
#define PRIv4 "c%.3f, %.3f, %.3f, %.3f)"

#define SCNv2 "*c%f, %f)"
#define SCNv3 "*c%f, %f, %f)"
#define SCNv4 "*c%f, %f, %f, %f)"

#define FMTv2(_v) '(', _v.x, _v.y
#define FMTv3(_v) '(', _v.x, _v.y, _v.z
#define FMTv4(_v) '(', _v.x, _v.y, _v.z, _v.w

#define PSCNv2(_v) &(_v).x, &(_v).y
#define PSCNv3(_v) &(_v).x, &(_v).y, &(_v).z
#define PSCNv4(_v) &(_v).x, &(_v).y, &(_v).z, &(_v).w

// use like PRIu**, fx.
// printf("%" PRIv2i, FMTv2i(vector));
#define PRIv2i "c%d, %d)"
#define PRIv3i "c%d, %d, %d)"
#define PRIv4i "c%d, %d, %d, %d)"

#define SCNv2i "*c%d, %d)"
#define SCNv3i "*c%d, %d, %d)"
#define SCNv4i "*c%d, %d, %d, %d)"

#define FMTv2i(_v) FMTv2(_v)
#define FMTv3i(_v) FMTv3(_v)
#define FMTv4i(_v) FMTv4(_v)

#define PSCNv2i(_v) &(_v).x, &(_v).y
#define PSCNv3i(_v) &(_v).x, &(_v).y, &(_v).z
#define PSCNv4i(_v) &(_v).x, &(_v).y, &(_v).z, &(_v).w

#define PRIm4                                                                  \
    "c[ %.3f %.3f %.3f %.3f ]\n"                                               \
    " [ %.3f %.3f %.3f %.3f ]\n"                                               \
    " [ %.3f %.3f %.3f %.3f ]\n"                                               \
    " [ %.3f %.3f %.3f %.3f ]]\n"                                              \

#define FMTm4(_m) '[',                                                         \
    _m.m00, _m.m10, _m.m20, _m.m30,                                            \
    _m.m01, _m.m12, _m.m21, _m.m31,                                            \
    _m.m02, _m.m12, _m.m22, _m.m32,                                            \
    _m.m03, _m.m13, _m.m23, _m.m33

// int vec -> float vec -> conversion functions
M_INLINE v2 v2_from_i(v2i i) { return v2_of(i.x, i.y); }
M_INLINE v3 v3_from_i(v3i i) { return v3_of(i.x, i.y, i.z); }
M_INLINE v4 v4_from_i(v4i i) { return v4_of(i.x, i.y, i.z, i.w); }

// float vec -> int vec conversion functions
M_INLINE v2i v2i_from_v(v2 i) { return v2i_of(i.x, i.y); }
M_INLINE v3i v3i_from_v(v3 i) { return v3i_of(i.x, i.y, i.z); }
M_INLINE v4i v4i_from_v(v4 i) { return v4i_of(i.x, i.y, i.z, i.w); }

#define v2_spread(v_) (v_).x, (v_).y
#define v3_spread(v_) (v_).x, (v_).y, (v_).z
#define v4_spread(v_) (v_).x, (v_).y, (v_).z, (v_).w
#define v2i_spread(v_) (v_).x, (v_).y
#define v3i_spread(v_) (v_).x, (v_).y, (v_).z
#define v4i_spread(v_) (v_).x, (v_).y, (v_).z, (v_).w

M_INLINE v2 v2_round(v2 v) {
    return v2_of(roundf(v.x), roundf(v.y));
}

M_INLINE v2i v2i_min(v2i v, v2i u) {
    return v2i_of(min(v.x, u.x), min(v.y, u.y));
}

M_INLINE v2i v2i_max(v2i v, v2i u) {
    return v2i_of(max(v.x, u.x), max(v.y, u.y));
}

M_INLINE v2i v2i_clampv(v2i v, v2i mi, v2i ma) {
    return v2i_of(clamp(v.x, mi.x, ma.x), clamp(v.y, mi.y, ma.y));
}

M_INLINE v2 v2_clampv(v2 v, v2 mi, v2 ma) {
    return v2_of(clamp(v.x, mi.x, ma.x), clamp(v.y, mi.y, ma.y));
}

M_INLINE v3 v3_clampv(v3 v, v3 mi, v3 ma) {
    return
        v3_of(
            clamp(v.x, mi.x, ma.x),
            clamp(v.y, mi.y, ma.y),
            clamp(v.z, mi.z, ma.z));
}

M_INLINE v4 v4_clampv(v4 v, v4 mi, v4 ma) {
    return
        v4_of(
            clamp(v.x, mi.x, ma.x),
            clamp(v.y, mi.y, ma.y),
            clamp(v.z, mi.z, ma.z),
            clamp(v.w, mi.w, ma.w));
}

M_INLINE v4 v4_reflect(v4 v, v4 n) {
  const v4 q = v4_scale(n, v4_dot(n, v) * 2.0f);
  return v4_sub(v, q);
}

M_INLINE v3 v3_reflect(v3 v, v3 n) {
    // r = v - (2(v dot n) * n)
    return v3_sub(v, v3_scale(n, v3_dot(v, n) * 2.0f));
}

typedef struct {
    f32 dist;
    v3 diff, dir;
} v3_diff_t;

// compute:
// dir: from -> to
// diff: to - from
// dist: length(to - from)
M_INLINE v3_diff_t v3_diff(v3 from, v3 to) {
    v3_diff_t d;
    d.diff = v3_sub(to, from);
    d.dist = v3_norm(d.diff);

    if (d.dist == 0.0f) {
        d.dir = v3_of(0);
    } else {
        d.dir = v3_scale(d.diff, 1.0f / d.dist);
    }

    return d;
}

typedef struct {
    f32 dist;
    v2 diff, dir;
} v2_diff_t;

// compute:
// dir: from -> to
// diff: to - from
// dist: length(to - from)
M_INLINE v2_diff_t v2_diff(v2 from, v2 to) {
    v2_diff_t d;
    d.diff = v2_sub(to, from);
    d.dist = v2_norm(d.diff);

    if (d.dist == 0.0f) {
        d.dir = v2_of(0);
    } else {
        d.dir = v2_scale(d.diff, 1.0f / d.dist);
    }

    return d;
}

// direction from one vector to another
M_INLINE v3 v3_dir(v3 from, v3 to) {
    return v3_normalize(v3_sub(to, from));
}

// direction from one vector to another
M_INLINE v2 v2_dir(v2 from, v2 to) {
    return v2_normalize(v2_sub(to, from));
}

// normalized 2D lerp
M_INLINE v2 v2_nlerp(v2 from, v2 to, f32 t) {
    return v2_normalize(v2_lerp(from, to, t));
}

// normalized 3D lerp
M_INLINE v3 v3_nlerp(v3 from, v3 to, f32 t) {
    return v3_normalize(v3_lerp(from, to, t));
}

// spherical 2D lerp
M_INLINE v2 v2_slerp(v2 from, v2 to, f32 t) {
    const f32
        dot = saturate(v2_dot(from, to)),
        theta = acosf(dot) * t;
    const v2 rel = v2_normalize(v2_sub(to, v2_scale(from, dot)));
    return
        v2_add(
            v2_scale(from, cosf(theta)), v2_scale(rel, sinf(theta)));
}

// dt respective spherical 2D lerp
M_INLINE v2 v2_dtslerp(v2 from, v2 to, f32 f, f32 dt) {
    return v2_slerp(from, to, dtlerp_t(f, dt));
}

// spherical 3D lerp
M_INLINE v3 v3_slerp(v3 from, v3 to, f32 t) {
    const f32
        dot = saturate(v3_dot(from, to)),
        theta = acosf(dot) * t;
    const v3 rel = v3_normalize(v3_sub(to, v3_scale(from, dot)));
    return
        v3_add(
            v3_scale(from, cosf(theta)), v3_scale(rel, sinf(theta)));
}

// dt respective spherical 2D lerp
M_INLINE v3 v3_dtslerp(v3 from, v3 to, f32 f, f32 dt) {
    return v3_slerp(from, to, dtlerp_t(f, dt));
}

// compute atan2f for v2
M_INLINE f32 v2_atan(v2 dir) {
    return atan2f(dir.y, dir.x);
}

// clamp vector to be inside of magnitude
M_INLINE v2 v2_clamp_mag(v2 v, f32 mag) {
    const v2_diff_t d = v2_diff(v2_of(0), v);
    return d.dist > mag ? v2_scale(d.dir, mag) : v;
}

// clamp vector to be inside of magnitude
M_INLINE v3 v3_clamp_mag(v3 v, f32 mag) {
    const v3_diff_t d = v3_diff(v3_of(0), v);
    return d.dist > mag ? v3_scale(d.dir, mag) : v;
}

M_INLINE v2 v2_proj(v2 a, v2 b) {
    return v2_scale(b, v2_dot(a, b) / v2_norm2(b));
}

// project a onto b, but only onto positive part of b
M_INLINE v3 v3_posproj(v3 a, v3 b) {
    return
        v3_scale(
            v3_proj(a, b),
            max(v3_dot(v3_normalize(a), v3_normalize(b)), 0));
}

// project a onto b, but only onto positive part of b
M_INLINE v2 v2_posproj(v2 a, v2 b) {
    return
        v2_scale(
            v2_proj(a, b),
            max(v2_dot(v2_normalize(a), v2_normalize(b)), 0));
}

// dt respecting lerp
M_INLINE v3 v3_dtlerp(v3 a, v3 b, f32 f, f32 dt) {
    return v3_lerp(a, b, dtlerp_t(f, dt));
}

// dt respecting lerp
M_INLINE v2 v2_dtlerp(v2 a, v2 b, f32 f, f32 dt) {
    return v2_lerp(a, b, dtlerp_t(f, dt));
}

M_INLINE m4 m4_rotate_make_dir(v3 dir) {
    dir = v3_normalize(dir);

    const v3
        x = v3_normalize(v3_cross(v3_of(0, 0, 1), dir)),
        y = v3_normalize(v3_cross(dir, x)),
        z = dir;

    m4 m = m4_identity();
    m.col[0] = v4_of(x, 0);
    m.col[1] = v4_of(y, 0);
    m.col[2] = v4_of(z, 0);
    return m;
}

M_INLINE bool v3_any_nan(v3 v) {
    return isnan(v.x) || isnan(v.y) || isnan(v.z);
}

M_INLINE v2 v2_floor(v2 v) {
    return v2_of(floor(v.x), floor(v.y));
}

// clamp direction "v" to be within "theta" radians of "dir"
M_INLINE v3 v3_clamp_cone(v3 v, v3 dir, f32 theta) {
    const f32 diff = acosf(v3_dot(dir, v));

    if (diff > theta) {
        // clamp difference
        v = v3_slerp(dir, v, theta / diff);
    }

    return v;
}

// solve plane for Z at specified point
M_INLINE f32 plane_get_z(v4 plane, v2 point) {
    // solve ax + by + cz + d = 0 for z
    // z = (-d - ax - by) / c
    return (-plane.w - (plane.x * point.x) - (plane.y * point.y)) / plane.z;
}

M_INLINE v4 plane_normalize(v4 p) {
    const f32 mag = sqrtf(p.x * p.x + p.y * p.y + p.z * p.z);
    return v4_divs(p, mag);
}

// get point on plane p
M_INLINE v3 plane_coplanar_point(v4 p) {
    return v3_scale(v3_from(p), -p.w);
}

// classify a point relative to a plane
// >0 in front, <0 behind, =0 on
M_INLINE f32 plane_classify(v4 plane, v3 point) {
    return plane.x * point.x + plane.y * point.y + plane.z * point.z + plane.w;
}

// project a point onto a plane
M_INLINE v3 plane_project(v4 plane, v3 point) {
    const v3 n = v3_from(plane);
    const f32 t = (v3_dot(point, n) + plane.w) / v3_dot(n, n);
    return v3_sub(point, v3_scale(n, t));
}

// any vector in the plane for which this basis is formed can be expressed as
// a linear combination of bx, by - this is intuitive, since the other component
// of the basis (the plane's normal vector) would contribute "0" in a linear
// combination of the three vectors anyway.
typedef struct { v3 bx, by; } plane_basis_2d_t;

// compute a 2D basis for the specified plane
M_INLINE plane_basis_2d_t plane_basis_2d(v4 p) {
    // compute a vector orthogonal to p's normal, on p
    // vector is arbitrary, so just add a bit to the origin, project into the
    // plane, and compute the direction
    const v3
        n = v3_from(p),
        origin = plane_coplanar_point(p),
        offset =
            fabsf(v3_dot(n, v3_of(1.0f))) < 0.999f ?
                v3_of(1.0f)
                : v3_normalize(v3_of(1.0f, 0.0f, 1.0f)),
        o = v3_dir(origin, plane_project(p, v3_add(origin, offset)));

    plane_basis_2d_t basis;

    basis.bx = v3_normalize(v3_cross(o, n));
    basis.by = v3_normalize(v3_cross(n, basis.bx));

    return basis;
}

// project q into the 2D basis formed by "basis" for plane p
M_INLINE v2 plane_project_basis_2d(
    v4 p,
    plane_basis_2d_t basis,
    v3 q) {
    // TODO: this is not terribly effecient, since we always know that the
    // third component of the multiplication of the projected q and the
    // inverse of a will *always* be zero. but it (probably) is not worth the
    // time to factor out the inverse to take that into account.
    const m3 a = {
        .col[0] = basis.bx,
        .col[1] = basis.by,
        .col[2] = v3_from(p),
    };

    // invert a
    const m3 a_inv = m3_inv(a);

    // project q into p and express as a linear combination of basis bx, by
    return v2_from(m3_mulv(a_inv, plane_project(p, q)));
}

// compute plane from triangle
M_INLINE v4 plane_from_triangle(v3 a, v3 b, v3 c) {
    const v3 n =
        v3_normalize(v3_cross(v3_sub(b, a), v3_sub(c, a)));

    // solve ax + by + cd + d = 0 for d
    return v4_of(n, -v3_dot(n, a));
}

M_INLINE bool v2i_eqv(v2i a, v2i b) {
    return a.x == b.x && a.y == b.y;
}
