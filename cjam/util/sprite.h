#pragma once

#include "../ext/sokol.h"
#include "../util/types.h"
#include "../util/math.h"

enum {
    SPRITE_NO_FLAGS   = 0,
    SPRITE_FLIP_X     = 1 << 0,
    SPRITE_FLIP_Y     = 1 << 1,
    SPRITE_ROTATE_CW  = 1 << 2,
    SPRITE_ROTATE_CCW = 1 << 3,
};

typedef struct sprite_atlas {
    sg_image image;
    sg_sampler sampler;

    // size of atlas in sprites
    v2i size_sprites;

    // size of atlas in pixels
    v2i size_px;

    // size of each sprite in pixels
    v2i sprite_size_px;

    // size of each sprite in texels
    v2 sprite_size_tx;

    // texels/pixel
    v2 tx_per_px;
} sprite_atlas_t;

typedef struct sprite {
    v2 pos;
    v2i index;
    v4 color;
    f32 z;
    i32 flags;
} sprite_t;

typedef struct sprite_instance sprite_instance_t;

typedef struct sprite_batch {
    const sprite_atlas_t *atlas;
    DYNLIST(sprite_instance_t) sprites;
} sprite_batch_t;

void sprite_atlas_init(
    sprite_atlas_t *atlas,
    const char *path,
    v2i sprite_size_px);

void sprite_atlas_destroy(sprite_atlas_t *atlas);

// atlas ptr must be valid for batch lifetime
void sprite_batch_init(
    sprite_batch_t *batch,
    allocator_t *a,
    const sprite_atlas_t *atlas);

void sprite_batch_destroy(sprite_batch_t *batch);

// push sprite for drawing
void sprite_batch_push(sprite_batch_t *batch, const sprite_t *sprite);

// push subimage for drawing
void sprite_batch_push_subimage(
    sprite_batch_t *batch,
    const sprite_t *sprite,
    boxi_t box);

// * model is optional
// * does not clear/destroy batch
void sprite_batch_draw(
    const sprite_batch_t *batch,
    const m4 *model,
    const m4 *view,
    const m4 *proj);

void sprite_draw_direct(
    sg_image image,
    const boxi_t *box,
    v2 pos,
    f32 z,
    v4 color,
    int flags,
    const m4 *model,
    const m4 *view,
    const m4 *proj);

#ifdef UTIL_IMPL

#ifndef SOKOL_GFX_INCLUDED
    #ifdef CLANGD
        #include "../ext/sokol.h"
    #else
        #error please include sokol_gfx.h
    #endif
#endif // ifndef SOKOL_GFX_INCLUDED

#define SOKOL_SHDC_IMPL
#include "../shader/sprite.glsl.h"

#include "../reloadhost/reloadhost.h"

#include "../util/dynlist.h"
#include "../util/image.h"

typedef struct sprite_instance {
    v2 offset;
    v2 scale;
    v2 uv_min, uv_max;
    v4 color;
    f32 z;
    f32 flags; // i32_bits_to_f32
} sprite_instance_t;

typedef struct sprite_vertex {
    v2 position;
    v2 texcoord;
} sprite_vertex_t;

// PER-FRAME limit!
#define SPRITE_MAX_INSTANCES 65536

static struct {
    bool init;
    sg_buffer ibuf, vbuf, instbuf;
    sg_shader shd;
    sg_pipeline pip;
    sg_sampler smp;
} _sprite;

RELOAD_STATIC_GLOBAL(_sprite)

static void sprite_lazy_init() {
    if (_sprite.init) {
        return;
    }

    _sprite.init = true;

    _sprite.smp =
        sg_make_sampler(
            &(sg_sampler_desc) {
                .wrap_u = SG_WRAP_CLAMP_TO_EDGE,
                .wrap_v = SG_WRAP_CLAMP_TO_EDGE,
                .min_filter = SG_FILTER_NEAREST,
                .mag_filter = SG_FILTER_NEAREST,
            });

    static const u16 indices[] = { 2, 1, 0, 3, 2, 0 };

    static const sprite_vertex_t vertices[] = {
        // pos       // uv
        { v2_const(0.0f, 1.0f),  v2_const(0.0f, 1.0f), },
        { v2_const(1.0f, 1.0f),  v2_const(1.0f, 1.0f), },
        { v2_const(1.0f, 0.0f),  v2_const(1.0f, 0.0f), },
        { v2_const(0.0f, 0.0f),  v2_const(0.0f, 0.0f), },
    };

    _sprite.ibuf =
        sg_make_buffer(
            &(sg_buffer_desc) {
                .type = SG_BUFFERTYPE_INDEXBUFFER,
                .data = SG_RANGE(indices),
            });

    _sprite.vbuf =
        sg_make_buffer(
            &(sg_buffer_desc) {
                .type = SG_BUFFERTYPE_VERTEXBUFFER,
                .data = SG_RANGE(vertices),
            });

   _sprite.instbuf =
        sg_make_buffer(
            &(sg_buffer_desc) {
                .type = SG_BUFFERTYPE_VERTEXBUFFER,
                .usage = SG_USAGE_STREAM,
                .size = SPRITE_MAX_INSTANCES * sizeof(sprite_instance_t),
            });

    _sprite.shd = sg_make_shader(sprite_sprite_shader_desc(sg_query_backend()));
    _sprite.pip =
        sg_make_pipeline(
            &(sg_pipeline_desc) {
                .layout = {
                    .buffers[1].step_func = SG_VERTEXSTEP_PER_INSTANCE,
                    .buffers[1].stride = sizeof(sprite_instance_t),
                    .attrs = {
                        [ATTR_sprite_vs_a_position] = {
                            .format = SG_VERTEXFORMAT_FLOAT2,
                            .offset = offsetof(sprite_vertex_t, position),
                            .buffer_index = 0,
                        },
                        [ATTR_sprite_vs_a_texcoord0] = {
                            .format = SG_VERTEXFORMAT_FLOAT2,
                            .offset = offsetof(sprite_vertex_t, texcoord),
                            .buffer_index = 0,
                        },
                        [ATTR_sprite_vs_a_offset] = {
                            .format = SG_VERTEXFORMAT_FLOAT2,
                            .offset = offsetof(sprite_instance_t, offset),
                            .buffer_index = 1,
                        },
                        [ATTR_sprite_vs_a_scale] = {
                            .format = SG_VERTEXFORMAT_FLOAT2,
                            .offset = offsetof(sprite_instance_t, scale),
                            .buffer_index = 1,
                        },
                        [ATTR_sprite_vs_a_uvmin] = {
                            .format = SG_VERTEXFORMAT_FLOAT2,
                            .offset = offsetof(sprite_instance_t, uv_min),
                            .buffer_index = 1,
                        },
                        [ATTR_sprite_vs_a_uvmax] = {
                            .format = SG_VERTEXFORMAT_FLOAT2,
                            .offset = offsetof(sprite_instance_t, uv_max),
                            .buffer_index = 1,
                        },
                        // TODO: convert to UBYTE4
                        [ATTR_sprite_vs_a_color] = {
                            .format = SG_VERTEXFORMAT_FLOAT4,
                            .offset = offsetof(sprite_instance_t, color),
                            .buffer_index = 1,
                        },
                        [ATTR_sprite_vs_a_z] = {
                            .format = SG_VERTEXFORMAT_FLOAT,
                            .offset = offsetof(sprite_instance_t, z),
                            .buffer_index = 1,
                        },
                        [ATTR_sprite_vs_a_flags] = {
                            .format = SG_VERTEXFORMAT_FLOAT,
                            .offset = offsetof(sprite_instance_t, flags),
                            .buffer_index = 1,
                        },
                    }
                },
                .shader = _sprite.shd,
                .primitive_type = SG_PRIMITIVETYPE_TRIANGLES,
                .index_type = SG_INDEXTYPE_UINT16,
                .cull_mode = SG_CULLMODE_NONE, // TODO: cull
                .face_winding = SG_FACEWINDING_CCW, // TODO: correct?
                .colors[0].blend = {
                    .enabled = true,
                    .src_factor_rgb = SG_BLENDFACTOR_SRC_ALPHA,
                    .dst_factor_rgb = SG_BLENDFACTOR_ONE_MINUS_SRC_ALPHA,
                },
                .depth = {
                    .compare = SG_COMPAREFUNC_LESS_EQUAL,
                    .write_enabled = true,
                },
                .label = "sprite-pipeline",
            });
}

void sprite_atlas_init(
    sprite_atlas_t *atlas,
    const char *path,
    v2i sprite_size_px) {
    u8 *data;
    v2i size;

    int res;
    ASSERT(!(res = image_load_rgba(path, &data, &size, thread_scratch())));

    atlas->image =
        sg_make_image(
            &(sg_image_desc) {
                .type = SG_IMAGETYPE_2D,
                .width = size.x,
                .height = size.y,
                .usage = SG_USAGE_IMMUTABLE,
                .pixel_format = SG_PIXELFORMAT_RGBA8,
                .data.subimage[0][0] = { data, size.x * size.y * 4 },
            });

    atlas->sampler =
        sg_make_sampler(
            &(sg_sampler_desc) {
                .wrap_u = SG_WRAP_CLAMP_TO_EDGE,
                .wrap_v = SG_WRAP_CLAMP_TO_EDGE,
                .min_filter = SG_FILTER_NEAREST,
                .mag_filter = SG_FILTER_NEAREST,
            });

    ASSERT(size.x % sprite_size_px.x == 0);
    ASSERT(size.y % sprite_size_px.y == 0);

    atlas->sprite_size_px = sprite_size_px;
    atlas->size_px = size;
    atlas->size_sprites = v2i_div(size, atlas->sprite_size_px);
    atlas->tx_per_px = v2_div(v2_of(1), v2_from_i(size));
    atlas->sprite_size_tx =
        v2_mul(v2_from_i(atlas->sprite_size_px), atlas->tx_per_px);
}

void sprite_atlas_destroy(sprite_atlas_t *atlas) {
    sg_destroy_image(atlas->image);
    sg_destroy_sampler(atlas->sampler);
    *atlas = (sprite_atlas_t) { 0 };
}

void sprite_batch_init(
    sprite_batch_t *batch,
    allocator_t *a,
    const sprite_atlas_t *atlas) {
    *batch = (sprite_batch_t) {
        .atlas = atlas,
        .sprites = dynlist_create(sprite_instance_t, a),
    };
}

void sprite_batch_destroy(sprite_batch_t *batch) {
    dynlist_destroy(batch->sprites);
    *batch = (sprite_batch_t) { 0 };
}

void sprite_batch_push(sprite_batch_t *batch, const sprite_t *sprite) {
    const v2
        uv_min =
            v2_mul(
                v2_from_i(
                    v2i_mul(sprite->index, batch->atlas->sprite_size_px)),
                batch->atlas->tx_per_px),
        uv_max = v2_add(uv_min, batch->atlas->sprite_size_tx);

    *dynlist_push(batch->sprites) = (sprite_instance_t) {
        .offset = sprite->pos,
        .scale = v2_from_i(batch->atlas->sprite_size_px),
        .uv_min = uv_min,
        .uv_max = uv_max,
        .color = sprite->color,
        .z = sprite->z,
        .flags = i32_bits_to_f32(sprite->flags),
    };
}

void sprite_batch_push_subimage(
    sprite_batch_t *batch,
    const sprite_t *sprite,
    boxi_t box) {
    const v2
        uv_min = v2_mul(v2_from_i(box.min), batch->atlas->tx_per_px),
        uv_max = v2_mul(v2_add(v2_from_i(box.max), v2_of(1)), batch->atlas->tx_per_px);

    *dynlist_push(batch->sprites) = (sprite_instance_t) {
        .offset = sprite->pos,
        .scale = v2_from_i(boxi_size(box)),
        .uv_min = uv_min,
        .uv_max = uv_max,
        .color = sprite->color,
        .z = sprite->z,
        .flags = i32_bits_to_f32(sprite->flags),
    };
}

void sprite_batch_draw(
    const sprite_batch_t *batch,
    const m4 *model,
    const m4 *view,
    const m4 *proj) {
    sprite_lazy_init();

    // upload instances
    const int offset =
        sg_append_buffer(
            _sprite.instbuf,
            &sg_range_from_dynlist(batch->sprites));

    if (sg_query_buffer_overflow(_sprite.instbuf)) {
        WARN("not all sprites drawn, internal buffer overflow");
    }

    sg_apply_pipeline(_sprite.pip);
    sg_apply_bindings(
        &(sg_bindings) {
            .index_buffer = _sprite.ibuf,
            .vertex_buffers[0] = _sprite.vbuf,
            .vertex_buffers[1] = _sprite.instbuf,
            .vertex_buffer_offsets[1] = offset,
            .fs.images[0] = batch->atlas->image,
            .fs.samplers[0] = batch->atlas->sampler,
        });

    sprite_vs_params_t vs_params;

    const m4 identity = m4_identity();
    memcpy(vs_params.model, model ? model->raw : identity.raw, sizeof(*model));
    memcpy(vs_params.view, view, sizeof(*view));
    memcpy(vs_params.proj, proj, sizeof(*proj));

    sg_apply_uniforms(
        SG_SHADERSTAGE_VS,
        SLOT_sprite_vs_params,
        &SG_RANGE(vs_params));

    sg_draw(0, 6, dynlist_size(batch->sprites));
}

void sprite_draw_direct(
    sg_image image,
    const boxi_t *box,
    v2 pos,
    f32 z,
    v4 color,
    int flags,
    const m4 *model,
    const m4 *view,
    const m4 *proj) {
    sprite_lazy_init();

    sg_image_desc desc = sg_query_image_desc(image);

    v2 uv_min, uv_max;
    v2i size;

    if (box) {
        size = boxi_size(*box);

        const v2 scale = v2_div(v2_of(1), v2_of(desc.width, desc.height));
        uv_min = v2_mul(v2_from_i(box->min), scale);
        uv_max = v2_mul(v2_from_i(box->max), scale);
    } else {
        size = v2i_of(desc.width, desc.height);
        uv_min = v2_of(0);
        uv_max = v2_of(1);
    }

    const sprite_instance_t instance = {
        .offset = pos,
        .z = z,
        .scale = v2_from_i(size),
        .color = color,
        .flags = i32_bits_to_f32(flags),
        .uv_min = uv_min,
        .uv_max = uv_max,
    };

    // upload instance
    const int offset =
        sg_append_buffer(
            _sprite.instbuf,
            &SG_RANGE(instance));

    if (sg_query_buffer_overflow(_sprite.instbuf)) {
        WARN("internal buffer overflow");
    }

    sg_apply_pipeline(_sprite.pip);
    sg_apply_bindings(
        &(sg_bindings) {
            .index_buffer = _sprite.ibuf,
            .vertex_buffers[0] = _sprite.vbuf,
            .vertex_buffers[1] = _sprite.instbuf,
            .vertex_buffer_offsets[1] = offset,
            .fs.images[0] = image,
            .fs.samplers[0] = _sprite.smp,
        });

    sprite_vs_params_t vs_params;

    const m4 identity = m4_identity();
    memcpy(vs_params.model, model ? model->raw : identity.raw, sizeof(*model));
    memcpy(vs_params.view, view, sizeof(*view));
    memcpy(vs_params.proj, proj, sizeof(*proj));

    sg_apply_uniforms(
        SG_SHADERSTAGE_VS,
        SLOT_sprite_vs_params,
        &SG_RANGE(vs_params));

    sg_draw(0, 6, 1);
}

#endif // ifdef UTIL_IMPL
