#pragma once

#include "../macros.h"
#include "../types.h"
#include "../mem.h"
#include "linalg.h"
#include "util.h"

// 2D line type
typedef struct line2 { v2 a, b; } line2_t;

// returns true if three points are colinear within the specified epsilon
M_INLINE bool points_are_colinear(v2 p0, v2 p1, v2 p2, f32 eps) {
    return
        fabsf(
            ((p1.y - p0.y) * (p2.x - p1.x))
                - ((p2.y - p1.y) * (p1.x - p0.x)))
            < eps;
}

// returns true if segments p1 -> p2 and p3 -> p4 are colinear within the
// specified epsilon
M_INLINE bool segments_are_colinear(
    v2 p1, v2 p2, v2 p3, v2 p4, f32 eps) {
    const f32
        p2x_m_p1x = p2.x - p1.x,
        p2y_m_p1y = p2.y - p1.y;

    // use same principle as colinear points, just precompute a bit
    return
        fabsf(
            ((p1.y - p3.y) * p2x_m_p1x)
                - (p2y_m_p1y * (p1.x - p3.x))) < eps
        && fabsf(
            ((p1.y - p4.y) * p2x_m_p1x)
                - (p2y_m_p1y * (p1.x - p4.x))) < eps;
}

// true if p is in triangle a, b, c
M_INLINE bool point_in_triangle(v2 p, v2 a, v2 b, v2 c) {
    const f32 d = ((b.y - c.y) * (a.x - c.x) + (c.x - b.x) * (a.y - c.y));
    if (fabsf(d) < 0.000001f) { return false; }

    // TODO: can exit early here?
    const f32
        x = ((b.y - c.y) * (p.x - c.x)
              + (c.x - b.x) * (p.y - c.y)) / d,
        y = ((c.y - a.y) * (p.x - c.x)
              + (a.x - c.x) * (p.y - c.y)) / d,
        z = 1 - x - y;

    return (x > 0) && (y > 0) && (z > 0);
}

// get area of triangle formed by 3 lines
M_INLINE f32 triangle_area(v2 a, v2 b, v2 c) {
    // see https://math.stackexchange.com/questions/901819
    const m2 mat = {
        .m00 = (b.x - a.x), .m10 = (b.y - a.y),
        .m01 = (c.x - a.x), .m11 = (c.y - a.y),
    };

    return 0.5f * m2_det(mat);
}

// <0 right, 0 on, >0 left
M_INLINE f32 point_side(v2 p, v2 a, v2 b) {
    return -(((p.x - a.x) * (b.y - a.y)) - ((p.y - a.y) * (b.x - a.x)));
}

// get t on _a -> _b where _p is closest
M_INLINE f32 point_project_segment_t(v2 p, v2 a, v2 b) {
    const f32 _l2 = v2_norm2(v2_sub(a, b));
    if (_l2 < 0.000001f) { return 0.0f; }
    const f32 _t =
        v2_dot(
            (v2_of(p.x - a.x, p.y - a.y)),
            (v2_of(b.x - a.x, b.y - a.y ))) / _l2;
    return _t;
}

// project point _p onto line segment _a -> _b
M_INLINE v2 point_project_segment(v2 p, v2 a, v2 b) {
    const f32 l2 = v2_norm2(v2_sub(a, b));
    if (l2 < 0.000001f) { return a; }
    const f32 t =
        clamp(
            v2_dot(
                (v2_of(p.x - a.x, p.y - a.y)),
                (v2_of(b.x - a.x, b.y - a.y ))) / l2,
            0.0f, 1.0f);
    return v2_add(a, v2_scale(v2_sub(b, a), t));
}

// project point _p onto line _a -> _b
M_INLINE v2 point_project_line(v2 p, v2 a, v2 b) {
    const f32 l2 = v2_norm2(v2_sub(a, b));
    if (l2 < 0.000001f) { return a; }
    const f32 t =
        v2_dot(
            (v2_of(p.x - a.x, p.y - a.y)),
            (v2_of(b.x - a.x, b.y - a.y ))) / l2;
    return v2_add(a, v2_scale(v2_sub(b, a), t));
}

// distance from p to a -> b
M_INLINE f32 point_to_segment(v2 p, v2 a, v2 b) {
    return v2_norm(v2_sub(p, point_project_segment(p, a, b)));
}

// gives left side normal of line
M_INLINE v2 line_left_normal(v2 a, v2 b) {
    return v2_normalize(v2_of(-(b.y - a.y), b.x - a.x));
}

// see: https://en.wikipedia.org/wiki/Line–line_intersection
// intersect two infinite lines
bool intersect_lines(
    v2 a0, v2 a1, v2 b0, v2 b1, v2 *hit);

// intersect two line segments, returns false if no intersection exists
bool intersect_segs(
    v2 a0, v2 a1, v2 b0, v2 b1, v2 *hit, f32 *ta, f32 *tb);

// intersect line segment s0 -> s1 with box defined by b0, b1
bool intersect_seg_box(
    v2 s0, v2 s1, v2 b0, v2 b1, v2 *hit);

// intersect segment a -> b with plane p
bool intersect_seg_plane(v3 a, v3 b, v4 p, v3 *hit, f32 *t);

// intersect two planes to get a vector (point / direction)
bool intersect_planes(v4 a, v4 b, v3 *point, v3 *dir);

// compute point of intersection of 3 planes
v3 intersect_3_planes(v4 a, v4 b, v4 c);

// TODO: rewrite to return intersection points
// intersect circles (p0, r0) and (p1, r1)
bool intersect_circle_circle(v2 p0, f32 r0, v2 p1, f32 r1);

// sweep dynamic circle [pr]1 along v against static circle [pr]0
// [pr]0 are static, [pr]1 are dynamic along v
bool sweep_circle_circle(
    v2 p0, f32 r0, v2 p1, f32 r1, v2 v, f32 *t, v2 *resolved);

// sweep circle at p with radius r along vector d, collide line segment a -> b
bool sweep_circle_line_segment(
    v2 p,
    f32 r,
    v2 d,
    v2 a,
    v2 b,
    f32 *t_circle,
    f32 *t_segment,
    v2 *resolved);

// intersect circle (p, r) with segment (s0, s1)
// returns number of intersection points (up to 2) and resolved circle pos
int intersect_circle_seg(
    v2 p, f32 r, v2 s0, v2 s1, f32 *ts, v2 *p_resolved);

// intersect line with sphere, returns number of intersections
int intersect_line_sphere(
    v3 a, v3 b, v3 center, f32 r, v3 ps[2]);

typedef struct circle_triangle_intersect {
    bool circle_contains;
    bool triangle_contains;
    int n;
    v2 hits[6];
} circle_triangle_intersect_t;

// intersect circle (p, r) with triangle (a, b, c)
// returns up to 6 intersection points
bool intersect_circle_triangle(
    v2 p, f32 r, v2 a, v2 b, v2 c,
    circle_triangle_intersect_t *out);

// intersect two triangles (a, b, c) and (p, q, r)
bool intersect_triangle_triangle(
    v2 a, v2 b, v2 c, v2 p, v2 q, v2 r);

// returns true if ts makes a hole inside of vs
bool polygon_is_hole(
    v2 vs[][2],
    int n_vs,
    v2 ts[][2],
    int n_ts);

// check if point p is in polygon specified by n lines in vs
bool polygon_contains_point(v2 p, const v2 lines[][2], int n);

#ifdef UTIL_IMPL

bool intersect_lines(
    v2 a0, v2 a1, v2 b0, v2 b1, v2 *hit) {
    const f32 d =
        ((a0.x - a1.x) * (b0.y - b1.y)) - ((a0.y - a1.y) * (b0.x - b1.x));

    // check for colinearity
    if (fabsf(d) < 0.000001f) { return false; }

    const f32
        xl0 = v2_cross(a0, a1),
        xl1 = v2_cross(b0, b1);

    if (hit) {
        *hit = v2_of(
            ((xl0 * (b0.x - b1.x)) - ((a0.x - a1.x) * xl1)) / d,
            ((xl0 * (b0.y - b1.y)) - ((a0.y - a1.y) * xl1)) / d
        );
    }

    return true;
}

bool intersect_segs(
    v2 a0, v2 a1, v2 b0, v2 b1, v2 *hit, f32 *ta, f32 *tb) {
    const f32 d =
        ((a0.x - a1.x) * (b0.y - b1.y)) - ((a0.y - a1.y) * (b0.x - b1.x));

    // check for colinearity
    if (fabsf(d) < 0.000001f) { return false; }

    const f32 t =
        (((a0.x - b0.x) * (b0.y - b1.y)) - ((a0.y - b0.y) * (b0.x - b1.x))) / d;

    if (t < 0 || t > 1) { return false; }

    const f32 u =
        (((a0.x - b0.x) * (a0.y - a1.y)) - ((a0.y - b0.y) * (a0.x - a1.x))) / d;

    if (u < 0 || u > 1) { return false; }

    if (hit) {
        *hit =
            v2_of(
                a0.x + (t * (a1.x - a0.x)),
                a0.y + (t * (a1.y - a0.y)));
    }

    if (ta) { *ta = t; }
    if (tb) { *tb = u; }

    return true;
}

bool intersect_seg_box(
    v2 s0, v2 s1, v2 b0, v2 b1, v2 *hit) {
    if (b0.x > b1.x) { swap(b0.x, b1.x); }
    if (b0.y > b1.y) { swap(b0.y, b1.y); }

    v2 r;
    if (intersect_segs(
            s0, s1,
            (v2_of(b0.x, b0.y)), (v2_of(b0.x, b1.y)),
            &r, NULL, NULL)) {
        if (hit) { *hit = r; }
        return true;
    } else if (
        intersect_segs(
            s0, s1,
            (v2_of(b0.x, b1.y)), (v2_of(b1.x, b1.y)),
            &r, NULL, NULL)) {
        if (hit) { *hit = r; }
        return true;
    } else if (
        intersect_segs(
            s0, s1,
            (v2_of(b1.x, b1.y)), (v2_of(b1.x, b0.y)),
            &r, NULL, NULL)) {
        if (hit) { *hit = r; }
        return true;
    } else if (
        intersect_segs(
            s0, s1,
            (v2_of(b1.x, b0.y)), (v2_of(b0.x, b0.y)),
            &r, NULL, NULL)) {
        if (hit) { *hit = r; }
        return true;
    }

    return false;
}

bool intersect_seg_plane(v3 a, v3 b, v4 p, v3 *hit, f32 *t) {
    const f32 sa = plane_classify(p, a), sb = plane_classify(p, b);

    if (sign(sa) == sign(sb)) {
        return false;
    }

    if (hit || t) {
        const v3
            c = plane_coplanar_point(p),
            r = v3_normalize(v3_sub(b, a));
        const f32 u =
            (v3_dot(v3_from(p), c) - v3_dot(v3_from(p), a))
                / v3_dot(v3_from(p), r);
        const v3 h = v3_add(a, v3_scale(r, u));
        if (hit) { *hit = h; }
        if (t) { *t = v3_norm(v3_sub(h, a)) / v3_norm(v3_sub(b, a)); }
    }

    return true;
}

bool intersect_planes(v4 a, v4 b, v3 *point, v3 *dir) {
    const v3 d = v3_cross(v3_from(a), v3_from(b));

    const f32 det = v3_norm2(d);
    if (det < 0.000001f) {
        // parallel
        return false;
    }

    if (dir) {
        *dir = d;
    }

    if (point) {
        *point =
            v3_divs(
                v3_add(
                    v3_scale(v3_cross(d, v3_from(b)), a.w),
                    v3_scale(v3_cross(v3_from(a), d), b.w)),
                det);
    }

    return true;
}

v3 intersect_3_planes(v4 a, v4 b, v4 c) {
    const v3
        n1 = v3_from(a),
        n2 = v3_from(b),
        n3 = v3_from(c),
        x1 = plane_coplanar_point(a),
        x2 = plane_coplanar_point(b),
        x3 = plane_coplanar_point(c),
        f1 = v3_scale(v3_cross(n2, n3), v3_dot(x1, n1)),
        f2 = v3_scale(v3_cross(n3, n1), v3_dot(x2, n2)),
        f3 = v3_scale(v3_cross(n1, n2), v3_dot(x3, n3));

    m3 m;
    m.col[0] = n1;
    m.col[1] = n2;
    m.col[2] = n3;
    const f32 det = m3_det(m);

    const v3 sum = v3_add(f1, v3_add(f2, f3));
    return v3_divs(sum, det);
}

bool intersect_circle_circle(v2 p0, f32 r0, v2 p1, f32 r1) {
    // distance between centers must be between r0 - r1, r0 + r1
    const f32
        mi = fabsf(r0 - r1),
        ma = r0 + r1,
        dx = p1.x - p0.x,
        dy = p1.y - p0.y,
        d = (dx * dx) + (dy * dy);
    return mi <= d && d <= ma;
}

bool sweep_circle_circle(
    v2 p0, f32 r0, v2 p1, f32 r1, v2 v, f32 *t, v2 *resolved) {

    const v2
        a = p1,
        b = v2_add(p1, v),
        d = point_project_segment(p0, a, b);

    if (v2_norm(v2_sub(p0, d)) > r0 + r1) {
        return false;
    }

    if (!t && !resolved) { return true; }

    const f32 _t =
        v2_norm(v2_sub(d, a))
            / v2_norm(v2_sub(b, a));

    const v2 p0_to_p1 = v2_sub(p1, p0);
    if (v2_norm(p0_to_p1) < r0 + r1 + 0.001f) {
        if (t) { *t = 0.0f; }
        if (resolved) {
            // circles are inside of each other, move p1 back such that distance
            // is exactly r0 + r1
            const v2 d = v2_normalize(p0_to_p1);
            *resolved = v2_add(p0, v2_scale(d, r0 + r1 + 0.001f));
        }
    } else {
        if (t) {
            *t = _t;
        }

        if (resolved) {
            *resolved = v2_lerp(a, b, _t);
        }
    }

    return true;
}

bool sweep_circle_line_segment(
    v2 p,
    f32 r,
    v2 d,
    v2 a,
    v2 b,
    f32 *t_circle,
    f32 *t_segment,
    v2 *resolved) {
    const f32 t = saturate(point_project_segment_t(p, a, b));
    const v2 closest = v2_lerp(a, b, t);

    const v2 closest_to_p = v2_sub(p, closest);
    const f32 dist_sqr = v2_norm2(closest_to_p);

    const v2 normal = v2_normalize(v2_sub(closest, p));

    // circle is already touching line
    if (dist_sqr < r * r) {
        *t_circle = 0.0f;
        *t_segment = t;
        *resolved =
            v2_add(
                closest,
                v2_scale(
                    normal,
                    -r));
        return true;
    }

    const v2 q = v2_add(p, d);

    // circle does not hit but may be touching endpoint
    if (intersect_segs(a, b, p, q, NULL, NULL, NULL)) {
        const f32
            dot_start = v2_dot(v2_sub(a, p), d),
            dot_end = v2_dot(v2_sub(b, p), d);

        v2 endpoint;
        f32 ts;

        if (dot_start < 0.0f) {
            if (dot_end < 0.0f) {
                return false;
            }

            ts = 1.0f;
            endpoint = b;
        } else if (dot_end < 0.0f) {
            ts = 0.0f;
            endpoint = a;
        } else {
            endpoint = dot_start < dot_end ? a : b;
            ts = dot_start < dot_end ? 0.0f : 1.0f;
        }

        // collide with the endpoint, a 0-radius circle
        if (intersect_circle_circle(p, r, endpoint, 0.0f)) {
            *t_circle = 0.0f;
            *t_segment = ts;
            *resolved = p;
            return true;
        }

        return false;
    }

    // circle has swept collision with line, but it may stop directly on the it
    const v2 v_to_line = v2_proj(d, normal);

    const f32
        t_to_line = v2_norm(v_to_line),
        needed_t_to_line = sqrtf(dist_sqr) - r;

    // check if the actual travel (t_to_line) is less than the necessary travel
    // to touch
    if (t_to_line < needed_t_to_line) {
        return false;
    }

    // circle intersects line, not necessarily segment
    const f32
        t_hit = needed_t_to_line / t_to_line,
        t_end = point_project_segment_t(q, a, b);

    // find distance along line segment at the point of intersection
    const f32 t_int = lerp(t, t_end, t_hit);

    // point is within segment bounds and circle is moving towards line
    if (t_int >= 0.0f
        && t_int <= 1.0f
        && v2_dot(v_to_line, v2_sub(closest, p)) >= 0.0f) {
        *t_circle = t_hit;
        *t_segment = t_int;
        *resolved = v2_add(p, v2_scale(d, t_hit));
        return true;
    }

    // may have hit an endpoint
    const v2 endpoint = t > 1.0f ? b : a;

    if (intersect_circle_circle(p, r, endpoint, 0.0f)) {
        *t_circle = t_hit;
        *t_segment = clamp(t, 0.0f, 1.0f);
        *resolved = v2_add(p, v2_scale(d, t_hit));
        return true;
    }

    return false;
}

int intersect_circle_seg(
    v2 p, f32 r, v2 s0, v2 s1, f32 *ts, v2 *p_resolved) {
    // see stackoverflow.com/questions/1073336
    const v2
        d = v2_of(s1.x - s0.x, s1.y - s0.y),
        f = v2_of(s0.x - p.x, s0.y - p.y);

    const f32
        a = v2_dot(d, d),
        b = 2.0f * v2_dot(f, d),
        c = v2_dot(f, f) - (r * r),
        q = (b * b) - (4 * a * c);

    if (q < 0) {
        return 0;
    }

    const f32 r_q = sqrtf(q);
    f32 s, t0, t1;

    if (b >= 0) {
        s = (-b - r_q) / 2;
    } else {
        s = (-b + r_q) / 2;
    }

    t0 = s / a;
    t1 = c / s;

    int n = 0;

    if (t0 >= 0 && t0 <= 1) {
        ts[n++] = 1.0f - t0;
    }

    if (t1 >= 0 && t1 <= 1) {
        ts[n++] = 1.0f - t1;
    }

    if (n == 2 && ts[0] > ts[1]) {
        swap(ts[0], ts[1]);
    }

    if (n != 0 && p_resolved) {
        // resolve circle
        const v2 q = point_project_segment(p, s0, s1);
        const v2 q_to_p = v2_sub(p, q);
        const v2 d_q_to_p = v2_normalize(q_to_p);

        *p_resolved =
            v2_add(
                p,
                v2_scale(
                    d_q_to_p,
                    r - v2_norm(q_to_p)));
    }

    return n;
}

int intersect_line_sphere(
    v3 a, v3 b, v3 center, f32 r, v3 ps[2]) {
    const v3 c = center, p = a, v = v3_sub(b, a);

    const f32
        A = v3_dot(v, v),
        B = 2.0 * (p.x * v.x + p.y * v.y + p.z * v.z - v.x * c.x - v.y * c.y - v.z * c.z),
        C = p.x * p.x - 2 * p.x * c.x + c.x * c.x + p.y * p.y - 2 * p.y * c.y + c.y * c.y +
               p.z * p.z - 2 * p.z * c.z + c.z * c.z -  r * r;

    // discriminant
    const f32 dsc = B * B - 4 * A * C;

    if (dsc < 0) {
        return 0;
    }

    const f32
        sqrt_dsc = sqrtf(dsc),
        inv_two_a = 1.0f / (2.0f * A);

    const f32 t0 = (-B - sqrt_dsc) * inv_two_a;
    const v3 p0 = v3_lerp(a, b, t0);

    if (fabsf(dsc) < 0.000001f) {
        if (ps) { ps[0] = p0; ps[1] = v3_of(0); }
        return 1;
    }

    const f32 t1 = (-B + sqrt_dsc) * inv_two_a;
    const v3 p1 = v3_lerp(a, b, t1);

    if (ps) { ps[0] = p0; ps[1] = p1; }
    return 2;
}

bool intersect_circle_triangle(
    v2 p, f32 r, v2 a, v2 b, v2 c,
    circle_triangle_intersect_t *out) {
    if (out) { *out = (circle_triangle_intersect_t) { 0 }; }

    const v2 points[3] = { a, b, c };

    const f32 r_squared = r * r;

    // check if all points are inside of circle
    if (v2_distance2(p, a) <= r_squared
        && v2_distance2(p, b) <= r_squared
        && v2_distance2(p, c) <= r_squared) {
        if (out) { out->circle_contains = true; }
        return true;
    }

    // check if circle is contained in triangle by projecting circle center
    // onto each triangle side, if the distance is greater than the radius then
    // the triangle contains the circle
    bool triangle_contains = true;
    for (int i = 0; i < 3 && triangle_contains; i++) {
        if (point_to_segment(p, points[i], points[(i + 1) % 3]) > r) {
            triangle_contains = false;
        }
    }

    if (triangle_contains) {
        if (out) { out->triangle_contains = true; }
        return true;
    }

    // check for intersections between triangle lines and circle
    int hits = 0;
    for (int i = 0; i < 3; i++) {
        const v2 p0 = points[i], p1 = points[(i + 1) % 3];

        f32 ts[2];
        const int n = intersect_circle_seg(p, r, p0, p1, ts, NULL);

        if (n == 0) {
            continue;
        } else if (!out) {
            // quick return: there is an intersection but we don't need out info
            return true;
        } else {
            // track points into output
            for (int j = 0; j < n; j++) {
                out->hits[hits++] = v2_lerp(p0, p1, ts[j]);
            }
        }
    }

    if (out) { out->n = hits; }

    return hits > 0;
}

// TODO: replace this helper function with some form of a simple hash set
static bool vertices_contains(v2 needle, v2 *haystack, int n) {
    for (int i = 0; i < n; i++) {
        if (v2_eqv_eps(needle, haystack[i])) { return true; }
    }
    return false;
}

bool polygon_is_hole(
    v2 vs[][2],
    int n_vs,
    v2 ts[][2],
    int n_ts) {
    // ts form an inner hole iff the points directly next to them are all inside
    // of the polygon formed by { vs, ts }

    for (int i = 0; i < n_ts; i++) {
        const v2 p0 = ts[i][0], p1 = ts[i][1];

        // compute q, point on normal of p0 -> p1 (left side normal!)
        const v2
            normal = v2_normalize(v2_of(-(p1.y - p0.y), p1.x - p0.x)),
            midpoint = v2_lerp(p0, p1, 0.5f),
            q =
                v2_of(
                    midpoint.x + 0.01f * normal.x,
                    midpoint.y + 0.01f * normal.y);

        // TODO: not for big polygons
        // compute a point such that q->u is a point outside of the polygon
        const v2 u = v2_of(q.x - 1e6f, q.y);

        // hit vertices
        v2 hits[n_vs + n_ts];

        // do a point-in-polygon test with q -> u
        int n_hits = 0;

        // test ts
        for (int j = 0; j < n_ts; j++) {
            v2 hit;

            if (segments_are_colinear(q, u, ts[j][0], ts[j][1], 0.01f)) {
                if (!vertices_contains(ts[j][0], hits, n_hits)) {
                    hits[n_hits++] = ts[j][0];
                }

                if (!vertices_contains(ts[j][1], hits, n_hits)) {
                    hits[n_hits++] = ts[j][1];
                }
            } else if (
                intersect_segs(q, u, ts[j][0], ts[j][1], &hit, NULL, NULL)) {
                if (!vertices_contains(hit, hits, n_hits)) {
                    hits[n_hits++] = hit;
                }
            }
        }

        // test vs
        for (int j = 0; j < n_vs; j++) {
            v2 hit;
            if (segments_are_colinear(q, u, vs[j][0], vs[j][1], 0.01f)) {
                if (!vertices_contains(vs[j][0], hits, n_hits)) {
                    hits[n_hits++] = vs[j][0];
                }

                if (!vertices_contains(vs[j][1], hits, n_hits)) {
                    hits[n_hits++] = vs[j][1];
                }
            } else if (
                intersect_segs(q, u, vs[j][0], vs[j][1], &hit, NULL, NULL)) {
                if (!vertices_contains(hit, hits, n_hits)) {
                    hits[n_hits++] = hit;
                }
            }
        }

        // even number of intersections -> outside of polygon, polygon cannot
        // be hole
        if (n_hits % 2 == 0) {
            return false;
        }
    }

    return true;
}

bool polygon_contains_point(v2 p, const v2 lines[][2], int n) {
    // TODO: not for big polygons
    // compute a point such that p -> u is a point outside of the polygon
    const v2 q = v2_of(p.x - 1e6f, p.y);

    int n_hits = 0;
    v2 hits[n * 2];

    for (int i = 0; i < n; i++) {
        v2 hit;
        if (segments_are_colinear(p, q, lines[i][0], lines[i][1], 0.01f)) {
            for (int j = 0; j < 2; j++) {
                if (!vertices_contains(lines[i][j], hits, n_hits)) {
                    hits[n_hits++] = lines[i][j];
                }
            }
        } else if (
            intersect_segs(p, q, lines[i][0], lines[i][1], &hit, NULL, NULL)) {
            if (!vertices_contains(hit, hits, n_hits)) {
                hits[n_hits++] = hit;
            }
        }
    }

    // odd number of hits -> inside polygon
    return n_hits % 2 != 0;
}

#define itt_orient(p0, p1, p2) (-point_side(p0, p2, p1))

M_INLINE bool itt_test_vertex(
    v2 a, v2 b, v2 c, v2 p, v2 q, v2 r) {
    if (itt_orient(r, p, b) >= 0.0f) {
        if (itt_orient(r, q, b) <= 0.0f) {
            if (itt_orient(a, p, b) > 0.0f) {
                return itt_orient(a, q, b) <= 0.0f;
            } else {
                return
                    itt_orient(a, p, c) >= 0.0f
                    && itt_orient(b, c, p) >= 0.0f;
            }
        } else {
            return
                itt_orient(a, q, b) <= 0.0f
                && itt_orient(r, q, c) <= 0.0f
                && itt_orient(b, c, q) >= 0.0f;
        }
    } else {
        if (itt_orient(r, p, c) >= 0.0f) {
            if (itt_orient(b, c, r) >= 0.0f) {
                return itt_orient(a, p, c) >= 0.0f;
            } else {
                return
                    itt_orient(b, c, q) >= 0.0f
                    && itt_orient(r, c, q) >= 0.0f;
            }
        } else {
            return false;
        }
    }
}

M_INLINE bool itt_test_edge(
    v2 a, v2 b, v2 c, v2 p, v2 q, v2 r) {
    if (itt_orient(r, p, b) >= 0.0f) {
        if (itt_orient(a, p, b) >= 0.0f) {
            if (itt_orient(a, b, r) >= 0.0f) {
                return 1;
            } else {
                return 0;
            }
        } else {
            if (itt_orient(b, c, p) >= 0.0f) {
                if (itt_orient(c, a, p) >= 0.0f) {
                    return 1;
                } else {
                    return 0;
                }
            } else {
                return 0;
            }
        }
    } else {
        if (itt_orient(r, p, c) >= 0.0f) {
            if (itt_orient(a, p, c) >= 0.0f) {
                if (itt_orient(a, c, r) >= 0.0f) {
                    return 1;
                } else {
                    if (itt_orient(b, c, r) >= 0.0f) {
                        return 1;
                    } else {
                        return 0;
                    }
                }
            } else {
                return 0;
            }
        } else {
            return 0;
        }
    }
}

bool intersect_triangle_triangle(
    v2 a, v2 b, v2 c, v2 p, v2 q, v2 r) {
    if (itt_orient(p,q,a) >= 0.0f) {
        if (itt_orient(q,r,a) >= 0.0f) {
            if (itt_orient(r,p,a) >= 0.0f) {
                return true;
            } else {
                return itt_test_edge(a,b,c,p,q,r);
            }
        } else {
            if (itt_orient(r,p,a) >= 0.0f)  {
                return itt_test_edge(a,b,c,r,p,q);
            } else {
                return itt_test_vertex(a,b,c,p,q,r);
            }
        }
    } else {
        if (itt_orient(q,r,a) >= 0.0f) {
            if (itt_orient(r,p,a) >= 0.0f)  {
                return itt_test_edge(a,b,c,q,r,p);
            } else {
                return itt_test_vertex(a,b,c,q,r,p);
            }
        } else return itt_test_vertex(a,b,c,r,p,q);
    }
}

#endif // ifdef UTIL_IMPL
