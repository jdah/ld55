#pragma once

#include "math.h"

#define V4_FROM_ABGR(_argb) ({              \
    u32 __argb = (_argb);                   \
    v4_of(                                  \
         ((__argb >> 0) & 0xFF) / 255.0f,   \
         ((__argb >> 8) & 0xFF) / 255.0f,   \
         ((__argb >> 16) & 0xFF) / 255.0f,  \
         ((__argb >> 24) & 0xFF) / 255.0f); \
    })

M_INLINE v4 color_scale_rgb(v4 rgba, f32 s) {
    return v4_of(v3_scale(v3_from(rgba), s), rgba.a);
}

M_INLINE v3 color_rgb_to_xyz(v3 c) {
    v3 tmp;
    tmp.x = (c.x > 0.04045) ? pow((c.x + 0.055) / 1.055, 2.4) : c.x / 12.92;
    tmp.y = (c.y > 0.04045) ? pow((c.y + 0.055) / 1.055, 2.4) : c.y / 12.92;
    tmp.z = (c.z > 0.04045) ? pow((c.z + 0.055) / 1.055, 2.4) : c.z / 12.92;

    m3 mat;
    mat.col[0] = (v3) {{ 0.4124, 0.3576, 0.1805 }};
    mat.col[1] = (v3) {{ 0.2126, 0.7152, 0.0722 }};
    mat.col[2] = (v3) {{ 0.0193, 0.1192, 0.9505 }};
    return v3_scale(m3_mulv(mat, tmp), 100.0f);
}

M_INLINE v3 color_xyz_to_lab(v3 c) {
    const v3 n = v3_div(c, v3_of(95.047, 100, 108.883));
    v3 v;
    v.x = (n.x > 0.008856) ? pow(n.x, 1.0 / 3.0) : (7.787 * n.x) + (16.0 / 116.0);
    v.y = (n.y > 0.008856) ? pow(n.y, 1.0 / 3.0) : (7.787 * n.y) + (16.0 / 116.0);
    v.z = (n.z > 0.008856) ? pow(n.z, 1.0 / 3.0) : (7.787 * n.z) + (16.0 / 116.0);
    return v3_of(
        (116.0 * v.y) - 16.0,
        500.0 * (v.x - v.y),
        200.0 * (v.y - v.z));
}

M_INLINE v3 color_rgb_to_lab(v3 c) {
    const v3 lab = color_xyz_to_lab(color_rgb_to_xyz(c));
    return v3_of(
        lab.x / 100.0,
        0.5 + 0.5 * (lab.y / 127.0),
        0.5 + 0.5 * (lab.z / 127.0));
}

M_INLINE v3 color_lab_to_xyz(v3 c) {
    f32 fy = (c.x + 16.0) / 116.0;
    f32 fx = c.y / 500.0 + fy;
    f32 fz = fy - c.z / 200.0;
    return v3_of(
        95.047 * ((fx > 0.206897) ? fx * fx * fx : (fx - 16.0 / 116.0) / 7.787),
        100.000 * ((fy > 0.206897) ? fy * fy * fy : (fy - 16.0 / 116.0) / 7.787),
        108.883 * ((fz > 0.206897) ? fz * fz * fz : (fz - 16.0 / 116.0) / 7.787));
}

M_INLINE v3 color_xyz_to_rgb(v3 c) {
    const m3 mat = {
        .col = {
            {{ 3.2406, -1.5372, -0.4986 }},
            {{ -0.9689, 1.8758, 0.0415 }},
            {{ 0.0557, -0.2040, 1.0570 }}
        }
    };
    const v3 v = m3_mulv(mat, (v3_scale(c, 1.0f / 100.0f)));
    v3 r;
    r.x = (v.x > 0.0031308) ? ((1.055 * pow(v.x, (1.0 / 2.4))) - 0.055) : 12.92 * v.x;
    r.y = (v.y > 0.0031308) ? ((1.055 * pow(v.y, (1.0 / 2.4))) - 0.055) : 12.92 * v.y;
    r.z = (v.z > 0.0031308) ? ((1.055 * pow(v.z, (1.0 / 2.4))) - 0.055) : 12.92 * v.z;
    return r;
}

M_INLINE v3 color_lab_to_rgb(v3 c) {
    return
        color_xyz_to_rgb(
            color_lab_to_xyz(
                v3_of(
                    100.0 * c.x,
                    2.0 * 127.0 * (c.y - 0.5),
                    2.0 * 127.0 * (c.z - 0.5))));
}

M_INLINE v4 color_abgr_to_v(u32 c) {
    return
        v4_of(
            ((c >>  0) & 0xFF) / 255.0f,
            ((c >>  8) & 0xFF) / 255.0f,
            ((c >> 16) & 0xFF) / 255.0f,
            ((c >> 24) & 0xFF) / 255.0f);
}

M_INLINE v3 color_rgb_to_hsv(v3 rgb)
{
    f32 r = rgb.r, g = rgb.g, b = rgb.b;

    float K = 0.f;
    if (g < b)
    {
        swap(g, b);
        K = -1.f;
    }
    if (r < g)
    {
        swap(r, g);
        K = -2.f / 6.f - K;
    }

    const float chroma = r - (g < b ? g : b);

    v3 hsv;
    hsv.x = fabsf(K + (g - b) / (6.f * chroma + 1e-20f));
    hsv.y = chroma / (r + 1e-20f);
    hsv.z = r;
    return hsv;
}

// Convert hsv floats ([0-1],[0-1],[0-1]) to rgb floats ([0-1],[0-1],[0-1]), from Foley & van Dam p593
// also http://en.wikipedia.org/wiki/HSL_and_HSV
M_INLINE v3 color_hsv_to_rgb(v3 hsv) {
    f32 h = hsv.x, s = hsv.y, v = hsv.z;
    if (s == 0.0f)
    {
        // gray
        return v3_of(v);
    }

    h = fmodf(h, 1.0f) / (60.0f / 360.0f);
    int   i = (int)h;
    float f = h - (float)i;
    float p = v * (1.0f - s);
    float q = v * (1.0f - s * f);
    float t = v * (1.0f - s * (1.0f - f));

    f32 r, g, b;
    switch (i)
    {
    case 0: r = v; g = t; b = p; break;
    case 1: r = q; g = v; b = p; break;
    case 2: r = p; g = v; b = t; break;
    case 3: r = p; g = q; b = v; break;
    case 4: r = t; g = p; b = v; break;
    case 5: default: r = v; g = p; b = q; break;
    }
    return v3_of(r, g, b);
}

// offsets some RGB color with HSV in [-1, 1]
M_INLINE v3 color_offset_with_hsv(v3 rgb, v3 hsv) {
    v3 col_hsv = color_rgb_to_hsv(rgb);
    col_hsv.x = fabsf(fmodf(col_hsv.x + hsv.x, 1.0f));
    col_hsv.y = saturate(col_hsv.y + hsv.y);
    col_hsv.z = saturate(col_hsv.z + hsv.z);
    return color_hsv_to_rgb(col_hsv);
}
