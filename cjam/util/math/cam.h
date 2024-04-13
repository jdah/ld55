#pragma once

#include "../types.h"
#include "linalg.h"

enum {
    F3D_PLANE_LEFT = 0,
    F3D_PLANE_RIGHT,
    F3D_PLANE_BOTTOM,
    F3D_PLANE_TOP,
    F3D_PLANE_NEAR,
    F3D_PLANE_FAR,
};

// 3D view frustum planes
typedef struct frustum_3d {
    v4 planes[6];
} frustum_3d_t;

// get direction from position to as screen pixel pos
v2 cam_dir_to_pixel(v3 dir, v3 pos, const m4 *view_proj, v2i dims);

// get 6 bounding planes for view-proj matrix
void cam_view_proj_to_frustum(const m4 *mat, frustum_3d_t *out);

enum {
    F3D_POINTS_NBL = 0,
    F3D_POINTS_NBR,
    F3D_POINTS_NTR,
    F3D_POINTS_NTL,
    F3D_POINTS_FBL,
    F3D_POINTS_FBR,
    F3D_POINTS_FTR,
    F3D_POINTS_FTL,
};

// get 8 intersection points for view-proj matrix
void cam_view_proj_to_corners(const m4 *mat, v3 points[8]);

// remove the pitch (up/down tilt) from a view-proj matrix
m4 cam_view_proj_remove_pitch(const m4 *view_proj);

typedef enum {
    FRUSTUM_AABB_OUTSIDE    = 0, // (0 or false)
    FRUSTUM_AABB_INTERSECT  = 1, // (0 or false)
    FRUSTUM_AABB_INSIDE     = 2, // (0 or false)
} frustum_vs_boxi_result_e;

// should be able to use results as bool
frustum_vs_boxi_result_e frustum_3d_vs_boxi_3d(
    const frustum_3d_t *f,
    box3f_t box);

#ifdef UTIL_IMPL

#include "box.h"

v2 cam_dir_to_pixel(
    v3 dir,
    v3 pos,
    const m4 *view_proj,
    v2i dims) {

    v4 p = v4_of(v3_add(pos, dir), 1.0f);
    p = m4_mulv(*view_proj, p);
    p.x /= p.w;
    p.y /= p.w;
    p.z /= p.w;

    return
        v2_mul(
            v2_divs(v2_add(v2_of(p.x, p.y), v2_of(1)), 2.0f),
            v2_from_i(v2i_sub(dims, v2i_of(1))));
}

void cam_view_proj_to_frustum(
    const m4 *mat,
    frustum_3d_t *out) {
    for (int i = 4; i--;) { out->planes[0].raw[i] = mat->raw[i][3] + mat->raw[i][0]; }
    for (int i = 4; i--;) { out->planes[1].raw[i] = mat->raw[i][3] - mat->raw[i][0]; }
    for (int i = 4; i--;) { out->planes[2].raw[i] = mat->raw[i][3] + mat->raw[i][1]; }
    for (int i = 4; i--;) { out->planes[3].raw[i] = mat->raw[i][3] - mat->raw[i][1]; }
    for (int i = 4; i--;) { out->planes[4].raw[i] = mat->raw[i][3] + mat->raw[i][2]; }
    for (int i = 4; i--;) { out->planes[5].raw[i] = mat->raw[i][3] - mat->raw[i][2]; }

    for (int i = 0; i < 6; i++) {
        out->planes[i] = plane_normalize(out->planes[i]);
    }
}

void cam_view_proj_to_corners(
    const m4 *mat,
    v3 points[8]) {
    const v3 unit[8] = {
        [F3D_POINTS_NBL] = v3_of(-1, -1, -1),
        [F3D_POINTS_NBR] = v3_of(+1, -1, -1),
        [F3D_POINTS_NTR] = v3_of(+1, +1, -1),
        [F3D_POINTS_NTL] = v3_of(-1, +1, -1),
        [F3D_POINTS_FBL] = v3_of(-1, -1, +1),
        [F3D_POINTS_FBR] = v3_of(+1, -1, +1),
        [F3D_POINTS_FTR] = v3_of(+1, +1, +1),
        [F3D_POINTS_FTL] = v3_of(-1, +1, +1),
    };
    const m4 inv = m4_inv(*mat);
    for (int i = 0; i < 8; i++) {
        const v4 v = m4_mulv(inv, v4_of(unit[i], 1.0f));
        points[i] = v3_divs(v3_from(v), v.w);
    }
}

m4 cam_view_proj_remove_pitch(const m4 *view_proj) {
    m3 rot = m4_pick3(*view_proj);
    rot.m10 = 0.0f;
    rot.m11 = 1.0f;
    rot.m12 = 0.0f;

    m4 res = *view_proj;
    glm_mat4_ins3(rot.raw, res.raw); // TODO: ???
    return res;
}

frustum_vs_boxi_result_e frustum_3d_vs_boxi_3d(
    const frustum_3d_t *f,
    box3f_t box) {
    // implementation adapted from
    // gamedev.net/forums/topic/672043-perfect-boxi-frustum-intersection-test/
    frustum_vs_boxi_result_e result = FRUSTUM_AABB_INSIDE;

    for (int i = 0; i < 6; i++) {
        const v4 p = f->planes[i];

        // TODO: bv3 type (?)
        const v3i sgn = v3i_of(p.x > 0.0f, p.y > 0.0f, p.z > 0.0f);

        const v3 signed_min_max =
            v3_of(box.vs[sgn.x].x, box.vs[sgn.y].y, box.vs[sgn.z].z);

        // check if box is entirely outside of plane
        if (v3_dot(v3_from(p), signed_min_max) < -p.w) {
            return FRUSTUM_AABB_OUTSIDE;
        }

        // TODO: make this part of calculation optional? would save one
        // dot product and branch per-plane...
        const v3 inv_signed_min_max =
            v3_of(box.vs[1 - sgn.x].x, box.vs[1 - sgn.y].y, box.vs[1 - sgn.z].z);

        // check if box is entirely outside of plane
        if (v3_dot(v3_from(p), inv_signed_min_max) <= -p.w) {
            // intersects
            result = FRUSTUM_AABB_INTERSECT;
        }
    }

    return result;
}



#endif // ifndef UTIL_IMPL
