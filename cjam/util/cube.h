#pragma once

#include "math.h"

extern const v3 CUBE_VERTICES[8];
extern const v2 CUBE_UVS[6][4];
extern const v3 CUBE_NORMALS[6];
extern const u16 CUBE_INDICES[6][6];
extern const u16 CUBE_UNIQUE_INDICES[4];
extern const u16 CUBE_FACE_INDICES[6];

#ifdef UTIL_IMPL

const v3 CUBE_VERTICES[8] = {
    v3_const(-0.5f, -0.5f, -0.5f),
    v3_const(-0.5f, +0.5f, -0.5f),
    v3_const(+0.5f, +0.5f, -0.5f),
    v3_const(+0.5f, -0.5f, -0.5f),
    v3_const(-0.5f, -0.5f, +0.5f),
    v3_const(-0.5f, +0.5f, +0.5f),
    v3_const(+0.5f, +0.5f, +0.5f),
    v3_const(+0.5f, -0.5f, +0.5f)
};

const v2 CUBE_UVS[6][4] = {
    {
        v2_const(0, 0),
        v2_const(1, 1),
        v2_const(0, 1),
        v2_const(1, 0),
    },
    {
        v2_const(1, 1),
        v2_const(0, 0),
        v2_const(1, 0),
        v2_const(0, 1),
    },
    {
        v2_const(1, 0),
        v2_const(0, 1),
        v2_const(0, 0),
        v2_const(1, 1),
    },
    {
        v2_const(0, 1),
        v2_const(1, 0),
        v2_const(1, 1),
        v2_const(0, 0),
    },
    {
        v2_const(0, 1),
        v2_const(1, 0),
        v2_const(0, 0),
        v2_const(1, 1),
    },
    {
        v2_const(1, 0),
        v2_const(0, 1),
        v2_const(1, 1),
        v2_const(0, 0),
    },
};

const v3 CUBE_NORMALS[6] = {
    v3_of(+1, 0, 0),
    v3_of(-1, 0, 0),
    v3_of(0, +1, 0),
    v3_of(0, -1, 0),
    v3_of(0, 0, +1),
    v3_of(0, 0, -1),
};

const u16 CUBE_INDICES[6][6] = {
    // 3, 6, 7, 2
    { 3, 6, 7, 3, 2, 6, }, // (east  (+x))
    { 4, 1, 0, 4, 5, 1, }, // (west  (-x))
    { 1, 6, 2, 1, 5, 6, }, // (north (+y))
    { 4, 3, 7, 4, 0, 3, }, // (south (-y))
    { 7, 5, 4, 7, 6, 5, }, // (up    (+z))
    { 0, 2, 3, 0, 1, 2, }, // (down  (-z))
};

// indices, within each list of 6 cube indices, which represent are the 4
// unique vertices which make up each face
const u16 CUBE_UNIQUE_INDICES[4] = { 0, 1, 2, 4 };

// indices into emitted vertices which make up the two faces for a cube face
const u16 CUBE_FACE_INDICES[6] = { 0, 1, 2, 0, 3, 1 };

#endif // ifdef UTIL_IMPL
