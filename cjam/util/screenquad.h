#pragma once

#ifndef SOKOL_GFX_INCLUDED
    #ifdef CLANGD
        #include "../ext/sokol.h"
    #else
        #error please include sokol_gfx.h
    #endif
#endif // ifndef SOKOL_GFX_INCLUDED

#include "../util/types.h"
#include "../util/math.h"

#ifdef UTIL_IMPL

#define SOKOL_SHDC_IMPL
#include "../shader/screenquad.glsl.h"

#include "../reloadhost/reloadhost.h"

static struct {
    bool init;
    sg_shader shd;
    sg_pipeline pip;
    sg_buffer vbuf, ibuf;
    sg_sampler smp;
} _sq;

RELOAD_STATIC_GLOBAL(_sq)

void screenquad_draw(sg_image image) {
    if (!_sq.init) {
        _sq.init = true;

        static const f32 vertices[] = {
            // pos        // uv
            0.0f, 1.0f,  0.0f, 1.0f,
            1.0f, 1.0f,  1.0f, 1.0f,
            1.0f, 0.0f,   1.0f, 0.0f,
            0.0f, 0.0f,   0.0f, 0.0f,
        };

        static const u16 indices[] = {
            0, 1, 2,
            0, 2, 3
        };

        _sq.vbuf =
            sg_make_buffer(
                &(sg_buffer_desc) {
                    .type = SG_BUFFERTYPE_VERTEXBUFFER,
                    .data = SG_RANGE(vertices),
                });
        _sq.ibuf =
            sg_make_buffer(
                &(sg_buffer_desc) {
                    .type = SG_BUFFERTYPE_INDEXBUFFER,
                    .data = SG_RANGE(indices)
                });

        _sq.shd =
            sg_make_shader(
                screenquad_screenquad_shader_desc(
                    sg_query_backend()));

        _sq.pip = sg_make_pipeline(&(sg_pipeline_desc) {
            .shader = _sq.shd,
            .primitive_type = SG_PRIMITIVETYPE_TRIANGLES,
            .index_type = SG_INDEXTYPE_UINT16,
            .layout = {
                .attrs = {
                    [0].format = SG_VERTEXFORMAT_FLOAT2,
                    [1].format = SG_VERTEXFORMAT_FLOAT2,
                }
            },
            .depth = {
                .compare = SG_COMPAREFUNC_LESS_EQUAL,
                .write_enabled = true
            },
            .cull_mode = SG_CULLMODE_BACK,
            .colors[0].blend = {
                .enabled = true,
                .src_factor_rgb = SG_BLENDFACTOR_SRC_ALPHA,
                .dst_factor_rgb = SG_BLENDFACTOR_ONE_MINUS_SRC_ALPHA,
            },
            .label = "screenquad-pipeline",
        });

        _sq.smp =
            sg_make_sampler(
                &(sg_sampler_desc) {
                    .wrap_u = SG_WRAP_CLAMP_TO_EDGE,
                    .wrap_v = SG_WRAP_CLAMP_TO_EDGE,
                    .min_filter = SG_FILTER_NEAREST,
                    .mag_filter = SG_FILTER_NEAREST,
                });
    }

    const m4
        model = m4_identity(),
        view = m4_identity(),
        proj = cam_ortho(0.0f, 1.0f, 0.0f, 1.0f, -1.0f, 1.0f);

    screenquad_vs_params_t vs_parms;
    memcpy(vs_parms.model, &model, sizeof(model));
    memcpy(vs_parms.view, &view, sizeof(view));
    memcpy(vs_parms.proj, &proj, sizeof(proj));

    sg_apply_pipeline(_sq.pip);
    sg_apply_bindings(
        &(sg_bindings) {
            .fs.images[0] = image,
            .fs.samplers[0] = _sq.smp,
            .index_buffer = _sq.ibuf,
            .vertex_buffers[0] = _sq.vbuf,
        });

    sg_apply_uniforms(
        SG_SHADERSTAGE_VS,
        SLOT_screenquad_vs_params,
        SG_RANGE_REF(vs_parms));

    sg_draw(0, 6, 1);
}

#endif // ifdef UTIL_IMPL
