#pragma once
/*
    #version:1# (machine generated, don't edit!)

    Generated by sokol-shdc (https://github.com/floooh/sokol-tools)

    Cmdline: sokol-shdc --input cjam/shader/sprite.glsl --output cjam/shader/sprite.glsl.h --slang glsl300es:glsl330 -b

    Overview:

        Shader program 'sprite':
            Get shader desc: sprite_sprite_shader_desc(sg_query_backend());
            Vertex shader: vs
                Attribute slots:
                    ATTR_sprite_vs_a_position = 0
                    ATTR_sprite_vs_a_texcoord0 = 1
                    ATTR_sprite_vs_a_offset = 2
                    ATTR_sprite_vs_a_scale = 3
                    ATTR_sprite_vs_a_uvmin = 4
                    ATTR_sprite_vs_a_uvmax = 5
                    ATTR_sprite_vs_a_color = 6
                    ATTR_sprite_vs_a_z = 7
                    ATTR_sprite_vs_a_flags = 8
                Uniform block 'vs_params':
                    C struct: sprite_vs_params_t
                    Bind slot: SLOT_sprite_vs_params = 0
            Fragment shader: fs
                Image 'tex':
                    Type: SG_IMAGETYPE_2D
                    Sample Type: SG_IMAGESAMPLETYPE_FLOAT
                    Bind slot: SLOT_sprite_tex = 0
                Sampler 'smp':
                    Type: SG_SAMPLERTYPE_FILTERING
                    Bind slot: SLOT_sprite_smp = 0
                Image Sampler Pair 'tex_smp':
                    Image: tex
                    Sampler: smp


    Shader descriptor structs:

        sg_shader sprite = sg_make_shader(sprite_sprite_shader_desc(sg_query_backend()));

    Vertex attribute locations for vertex shader 'vs':

        sg_pipeline pip = sg_make_pipeline(&(sg_pipeline_desc){
            .layout = {
                .attrs = {
                    [ATTR_sprite_vs_a_position] = { ... },
                    [ATTR_sprite_vs_a_texcoord0] = { ... },
                    [ATTR_sprite_vs_a_offset] = { ... },
                    [ATTR_sprite_vs_a_scale] = { ... },
                    [ATTR_sprite_vs_a_uvmin] = { ... },
                    [ATTR_sprite_vs_a_uvmax] = { ... },
                    [ATTR_sprite_vs_a_color] = { ... },
                    [ATTR_sprite_vs_a_z] = { ... },
                    [ATTR_sprite_vs_a_flags] = { ... },
                },
            },
            ...});


    Image bind slots, use as index in sg_bindings.vs.images[] or .fs.images[]

        SLOT_sprite_tex = 0;

    Sampler bind slots, use as index in sg_bindings.vs.sampler[] or .fs.samplers[]

        SLOT_sprite_smp = 0;

    Bind slot and C-struct for uniform block 'vs_params':

        sprite_vs_params_t vs_params = {
            .model = ...;
            .view = ...;
            .proj = ...;
        };
        sg_apply_uniforms(SG_SHADERSTAGE_[VS|FS], SLOT_sprite_vs_params, &SG_RANGE(vs_params));

*/
#include <stdint.h>
#include <stdbool.h>
#include <string.h>
#include <stddef.h>
#if !defined(SOKOL_SHDC_ALIGN)
  #if defined(_MSC_VER)
    #define SOKOL_SHDC_ALIGN(a) __declspec(align(a))
  #else
    #define SOKOL_SHDC_ALIGN(a) __attribute__((aligned(a)))
  #endif
#endif
#define ATTR_sprite_vs_a_position (0)
#define ATTR_sprite_vs_a_texcoord0 (1)
#define ATTR_sprite_vs_a_offset (2)
#define ATTR_sprite_vs_a_scale (3)
#define ATTR_sprite_vs_a_uvmin (4)
#define ATTR_sprite_vs_a_uvmax (5)
#define ATTR_sprite_vs_a_color (6)
#define ATTR_sprite_vs_a_z (7)
#define ATTR_sprite_vs_a_flags (8)
#define SLOT_sprite_tex (0)
#define SLOT_sprite_smp (0)
#define SLOT_sprite_vs_params (0)
#pragma pack(push,1)
SOKOL_SHDC_ALIGN(16) typedef struct sprite_vs_params_t {
    float model[16];
    float view[16];
    float proj[16];
} sprite_vs_params_t;
#pragma pack(pop)
/*
    #version 330
    
    uniform vec4 vs_params[12];
    layout(location = 1) in vec2 a_texcoord0;
    layout(location = 8) in float a_flags;
    out vec2 uv;
    layout(location = 4) in vec2 a_uvmin;
    layout(location = 5) in vec2 a_uvmax;
    out vec4 color;
    layout(location = 6) in vec4 a_color;
    out float depth;
    layout(location = 7) in float a_z;
    layout(location = 2) in vec2 a_offset;
    layout(location = 3) in vec2 a_scale;
    layout(location = 0) in vec2 a_position;
    
    void main()
    {
        vec2 texcoord = a_texcoord0;
        int _19 = floatBitsToInt(a_flags);
        if ((_19 & 1) != 0)
        {
            vec2 _101 = texcoord;
            _101.x = 1.0 - _101.x;
            texcoord = _101;
        }
        if ((_19 & 2) != 0)
        {
            vec2 _104 = texcoord;
            _104.y = 1.0 - _104.y;
            texcoord = _104;
        }
        uv = texcoord * (a_uvmax - a_uvmin) + a_uvmin;
        color = a_color;
        depth = a_z;
        gl_Position = ((mat4(vs_params[8], vs_params[9], vs_params[10], vs_params[11]) * mat4(vs_params[4], vs_params[5], vs_params[6], vs_params[7])) * mat4(vs_params[0], vs_params[1], vs_params[2], vs_params[3])) * vec4(a_scale * a_position + a_offset, a_z, 1.0);
    }
    
*/
static const char sprite_vs_source_glsl330[1122] = {
    0x23,0x76,0x65,0x72,0x73,0x69,0x6f,0x6e,0x20,0x33,0x33,0x30,0x0a,0x0a,0x75,0x6e,
    0x69,0x66,0x6f,0x72,0x6d,0x20,0x76,0x65,0x63,0x34,0x20,0x76,0x73,0x5f,0x70,0x61,
    0x72,0x61,0x6d,0x73,0x5b,0x31,0x32,0x5d,0x3b,0x0a,0x6c,0x61,0x79,0x6f,0x75,0x74,
    0x28,0x6c,0x6f,0x63,0x61,0x74,0x69,0x6f,0x6e,0x20,0x3d,0x20,0x31,0x29,0x20,0x69,
    0x6e,0x20,0x76,0x65,0x63,0x32,0x20,0x61,0x5f,0x74,0x65,0x78,0x63,0x6f,0x6f,0x72,
    0x64,0x30,0x3b,0x0a,0x6c,0x61,0x79,0x6f,0x75,0x74,0x28,0x6c,0x6f,0x63,0x61,0x74,
    0x69,0x6f,0x6e,0x20,0x3d,0x20,0x38,0x29,0x20,0x69,0x6e,0x20,0x66,0x6c,0x6f,0x61,
    0x74,0x20,0x61,0x5f,0x66,0x6c,0x61,0x67,0x73,0x3b,0x0a,0x6f,0x75,0x74,0x20,0x76,
    0x65,0x63,0x32,0x20,0x75,0x76,0x3b,0x0a,0x6c,0x61,0x79,0x6f,0x75,0x74,0x28,0x6c,
    0x6f,0x63,0x61,0x74,0x69,0x6f,0x6e,0x20,0x3d,0x20,0x34,0x29,0x20,0x69,0x6e,0x20,
    0x76,0x65,0x63,0x32,0x20,0x61,0x5f,0x75,0x76,0x6d,0x69,0x6e,0x3b,0x0a,0x6c,0x61,
    0x79,0x6f,0x75,0x74,0x28,0x6c,0x6f,0x63,0x61,0x74,0x69,0x6f,0x6e,0x20,0x3d,0x20,
    0x35,0x29,0x20,0x69,0x6e,0x20,0x76,0x65,0x63,0x32,0x20,0x61,0x5f,0x75,0x76,0x6d,
    0x61,0x78,0x3b,0x0a,0x6f,0x75,0x74,0x20,0x76,0x65,0x63,0x34,0x20,0x63,0x6f,0x6c,
    0x6f,0x72,0x3b,0x0a,0x6c,0x61,0x79,0x6f,0x75,0x74,0x28,0x6c,0x6f,0x63,0x61,0x74,
    0x69,0x6f,0x6e,0x20,0x3d,0x20,0x36,0x29,0x20,0x69,0x6e,0x20,0x76,0x65,0x63,0x34,
    0x20,0x61,0x5f,0x63,0x6f,0x6c,0x6f,0x72,0x3b,0x0a,0x6f,0x75,0x74,0x20,0x66,0x6c,
    0x6f,0x61,0x74,0x20,0x64,0x65,0x70,0x74,0x68,0x3b,0x0a,0x6c,0x61,0x79,0x6f,0x75,
    0x74,0x28,0x6c,0x6f,0x63,0x61,0x74,0x69,0x6f,0x6e,0x20,0x3d,0x20,0x37,0x29,0x20,
    0x69,0x6e,0x20,0x66,0x6c,0x6f,0x61,0x74,0x20,0x61,0x5f,0x7a,0x3b,0x0a,0x6c,0x61,
    0x79,0x6f,0x75,0x74,0x28,0x6c,0x6f,0x63,0x61,0x74,0x69,0x6f,0x6e,0x20,0x3d,0x20,
    0x32,0x29,0x20,0x69,0x6e,0x20,0x76,0x65,0x63,0x32,0x20,0x61,0x5f,0x6f,0x66,0x66,
    0x73,0x65,0x74,0x3b,0x0a,0x6c,0x61,0x79,0x6f,0x75,0x74,0x28,0x6c,0x6f,0x63,0x61,
    0x74,0x69,0x6f,0x6e,0x20,0x3d,0x20,0x33,0x29,0x20,0x69,0x6e,0x20,0x76,0x65,0x63,
    0x32,0x20,0x61,0x5f,0x73,0x63,0x61,0x6c,0x65,0x3b,0x0a,0x6c,0x61,0x79,0x6f,0x75,
    0x74,0x28,0x6c,0x6f,0x63,0x61,0x74,0x69,0x6f,0x6e,0x20,0x3d,0x20,0x30,0x29,0x20,
    0x69,0x6e,0x20,0x76,0x65,0x63,0x32,0x20,0x61,0x5f,0x70,0x6f,0x73,0x69,0x74,0x69,
    0x6f,0x6e,0x3b,0x0a,0x0a,0x76,0x6f,0x69,0x64,0x20,0x6d,0x61,0x69,0x6e,0x28,0x29,
    0x0a,0x7b,0x0a,0x20,0x20,0x20,0x20,0x76,0x65,0x63,0x32,0x20,0x74,0x65,0x78,0x63,
    0x6f,0x6f,0x72,0x64,0x20,0x3d,0x20,0x61,0x5f,0x74,0x65,0x78,0x63,0x6f,0x6f,0x72,
    0x64,0x30,0x3b,0x0a,0x20,0x20,0x20,0x20,0x69,0x6e,0x74,0x20,0x5f,0x31,0x39,0x20,
    0x3d,0x20,0x66,0x6c,0x6f,0x61,0x74,0x42,0x69,0x74,0x73,0x54,0x6f,0x49,0x6e,0x74,
    0x28,0x61,0x5f,0x66,0x6c,0x61,0x67,0x73,0x29,0x3b,0x0a,0x20,0x20,0x20,0x20,0x69,
    0x66,0x20,0x28,0x28,0x5f,0x31,0x39,0x20,0x26,0x20,0x31,0x29,0x20,0x21,0x3d,0x20,
    0x30,0x29,0x0a,0x20,0x20,0x20,0x20,0x7b,0x0a,0x20,0x20,0x20,0x20,0x20,0x20,0x20,
    0x20,0x76,0x65,0x63,0x32,0x20,0x5f,0x31,0x30,0x31,0x20,0x3d,0x20,0x74,0x65,0x78,
    0x63,0x6f,0x6f,0x72,0x64,0x3b,0x0a,0x20,0x20,0x20,0x20,0x20,0x20,0x20,0x20,0x5f,
    0x31,0x30,0x31,0x2e,0x78,0x20,0x3d,0x20,0x31,0x2e,0x30,0x20,0x2d,0x20,0x5f,0x31,
    0x30,0x31,0x2e,0x78,0x3b,0x0a,0x20,0x20,0x20,0x20,0x20,0x20,0x20,0x20,0x74,0x65,
    0x78,0x63,0x6f,0x6f,0x72,0x64,0x20,0x3d,0x20,0x5f,0x31,0x30,0x31,0x3b,0x0a,0x20,
    0x20,0x20,0x20,0x7d,0x0a,0x20,0x20,0x20,0x20,0x69,0x66,0x20,0x28,0x28,0x5f,0x31,
    0x39,0x20,0x26,0x20,0x32,0x29,0x20,0x21,0x3d,0x20,0x30,0x29,0x0a,0x20,0x20,0x20,
    0x20,0x7b,0x0a,0x20,0x20,0x20,0x20,0x20,0x20,0x20,0x20,0x76,0x65,0x63,0x32,0x20,
    0x5f,0x31,0x30,0x34,0x20,0x3d,0x20,0x74,0x65,0x78,0x63,0x6f,0x6f,0x72,0x64,0x3b,
    0x0a,0x20,0x20,0x20,0x20,0x20,0x20,0x20,0x20,0x5f,0x31,0x30,0x34,0x2e,0x79,0x20,
    0x3d,0x20,0x31,0x2e,0x30,0x20,0x2d,0x20,0x5f,0x31,0x30,0x34,0x2e,0x79,0x3b,0x0a,
    0x20,0x20,0x20,0x20,0x20,0x20,0x20,0x20,0x74,0x65,0x78,0x63,0x6f,0x6f,0x72,0x64,
    0x20,0x3d,0x20,0x5f,0x31,0x30,0x34,0x3b,0x0a,0x20,0x20,0x20,0x20,0x7d,0x0a,0x20,
    0x20,0x20,0x20,0x75,0x76,0x20,0x3d,0x20,0x74,0x65,0x78,0x63,0x6f,0x6f,0x72,0x64,
    0x20,0x2a,0x20,0x28,0x61,0x5f,0x75,0x76,0x6d,0x61,0x78,0x20,0x2d,0x20,0x61,0x5f,
    0x75,0x76,0x6d,0x69,0x6e,0x29,0x20,0x2b,0x20,0x61,0x5f,0x75,0x76,0x6d,0x69,0x6e,
    0x3b,0x0a,0x20,0x20,0x20,0x20,0x63,0x6f,0x6c,0x6f,0x72,0x20,0x3d,0x20,0x61,0x5f,
    0x63,0x6f,0x6c,0x6f,0x72,0x3b,0x0a,0x20,0x20,0x20,0x20,0x64,0x65,0x70,0x74,0x68,
    0x20,0x3d,0x20,0x61,0x5f,0x7a,0x3b,0x0a,0x20,0x20,0x20,0x20,0x67,0x6c,0x5f,0x50,
    0x6f,0x73,0x69,0x74,0x69,0x6f,0x6e,0x20,0x3d,0x20,0x28,0x28,0x6d,0x61,0x74,0x34,
    0x28,0x76,0x73,0x5f,0x70,0x61,0x72,0x61,0x6d,0x73,0x5b,0x38,0x5d,0x2c,0x20,0x76,
    0x73,0x5f,0x70,0x61,0x72,0x61,0x6d,0x73,0x5b,0x39,0x5d,0x2c,0x20,0x76,0x73,0x5f,
    0x70,0x61,0x72,0x61,0x6d,0x73,0x5b,0x31,0x30,0x5d,0x2c,0x20,0x76,0x73,0x5f,0x70,
    0x61,0x72,0x61,0x6d,0x73,0x5b,0x31,0x31,0x5d,0x29,0x20,0x2a,0x20,0x6d,0x61,0x74,
    0x34,0x28,0x76,0x73,0x5f,0x70,0x61,0x72,0x61,0x6d,0x73,0x5b,0x34,0x5d,0x2c,0x20,
    0x76,0x73,0x5f,0x70,0x61,0x72,0x61,0x6d,0x73,0x5b,0x35,0x5d,0x2c,0x20,0x76,0x73,
    0x5f,0x70,0x61,0x72,0x61,0x6d,0x73,0x5b,0x36,0x5d,0x2c,0x20,0x76,0x73,0x5f,0x70,
    0x61,0x72,0x61,0x6d,0x73,0x5b,0x37,0x5d,0x29,0x29,0x20,0x2a,0x20,0x6d,0x61,0x74,
    0x34,0x28,0x76,0x73,0x5f,0x70,0x61,0x72,0x61,0x6d,0x73,0x5b,0x30,0x5d,0x2c,0x20,
    0x76,0x73,0x5f,0x70,0x61,0x72,0x61,0x6d,0x73,0x5b,0x31,0x5d,0x2c,0x20,0x76,0x73,
    0x5f,0x70,0x61,0x72,0x61,0x6d,0x73,0x5b,0x32,0x5d,0x2c,0x20,0x76,0x73,0x5f,0x70,
    0x61,0x72,0x61,0x6d,0x73,0x5b,0x33,0x5d,0x29,0x29,0x20,0x2a,0x20,0x76,0x65,0x63,
    0x34,0x28,0x61,0x5f,0x73,0x63,0x61,0x6c,0x65,0x20,0x2a,0x20,0x61,0x5f,0x70,0x6f,
    0x73,0x69,0x74,0x69,0x6f,0x6e,0x20,0x2b,0x20,0x61,0x5f,0x6f,0x66,0x66,0x73,0x65,
    0x74,0x2c,0x20,0x61,0x5f,0x7a,0x2c,0x20,0x31,0x2e,0x30,0x29,0x3b,0x0a,0x7d,0x0a,
    0x0a,0x00,
};
/*
    #version 330
    
    uniform sampler2D tex_smp;
    
    layout(location = 0) out vec4 frag_color;
    in vec4 color;
    in vec2 uv;
    in float depth;
    
    void main()
    {
        frag_color = color * texture(tex_smp, uv);
        if (frag_color.w < 9.9999997473787516355514526367188e-05)
        {
            discard;
        }
    }
    
*/
static const char sprite_fs_source_glsl330[284] = {
    0x23,0x76,0x65,0x72,0x73,0x69,0x6f,0x6e,0x20,0x33,0x33,0x30,0x0a,0x0a,0x75,0x6e,
    0x69,0x66,0x6f,0x72,0x6d,0x20,0x73,0x61,0x6d,0x70,0x6c,0x65,0x72,0x32,0x44,0x20,
    0x74,0x65,0x78,0x5f,0x73,0x6d,0x70,0x3b,0x0a,0x0a,0x6c,0x61,0x79,0x6f,0x75,0x74,
    0x28,0x6c,0x6f,0x63,0x61,0x74,0x69,0x6f,0x6e,0x20,0x3d,0x20,0x30,0x29,0x20,0x6f,
    0x75,0x74,0x20,0x76,0x65,0x63,0x34,0x20,0x66,0x72,0x61,0x67,0x5f,0x63,0x6f,0x6c,
    0x6f,0x72,0x3b,0x0a,0x69,0x6e,0x20,0x76,0x65,0x63,0x34,0x20,0x63,0x6f,0x6c,0x6f,
    0x72,0x3b,0x0a,0x69,0x6e,0x20,0x76,0x65,0x63,0x32,0x20,0x75,0x76,0x3b,0x0a,0x69,
    0x6e,0x20,0x66,0x6c,0x6f,0x61,0x74,0x20,0x64,0x65,0x70,0x74,0x68,0x3b,0x0a,0x0a,
    0x76,0x6f,0x69,0x64,0x20,0x6d,0x61,0x69,0x6e,0x28,0x29,0x0a,0x7b,0x0a,0x20,0x20,
    0x20,0x20,0x66,0x72,0x61,0x67,0x5f,0x63,0x6f,0x6c,0x6f,0x72,0x20,0x3d,0x20,0x63,
    0x6f,0x6c,0x6f,0x72,0x20,0x2a,0x20,0x74,0x65,0x78,0x74,0x75,0x72,0x65,0x28,0x74,
    0x65,0x78,0x5f,0x73,0x6d,0x70,0x2c,0x20,0x75,0x76,0x29,0x3b,0x0a,0x20,0x20,0x20,
    0x20,0x69,0x66,0x20,0x28,0x66,0x72,0x61,0x67,0x5f,0x63,0x6f,0x6c,0x6f,0x72,0x2e,
    0x77,0x20,0x3c,0x20,0x39,0x2e,0x39,0x39,0x39,0x39,0x39,0x39,0x37,0x34,0x37,0x33,
    0x37,0x38,0x37,0x35,0x31,0x36,0x33,0x35,0x35,0x35,0x31,0x34,0x35,0x32,0x36,0x33,
    0x36,0x37,0x31,0x38,0x38,0x65,0x2d,0x30,0x35,0x29,0x0a,0x20,0x20,0x20,0x20,0x7b,
    0x0a,0x20,0x20,0x20,0x20,0x20,0x20,0x20,0x20,0x64,0x69,0x73,0x63,0x61,0x72,0x64,
    0x3b,0x0a,0x20,0x20,0x20,0x20,0x7d,0x0a,0x7d,0x0a,0x0a,0x00,
};
/*
    #version 300 es
    
    uniform vec4 vs_params[12];
    layout(location = 1) in vec2 a_texcoord0;
    layout(location = 8) in float a_flags;
    out vec2 uv;
    layout(location = 4) in vec2 a_uvmin;
    layout(location = 5) in vec2 a_uvmax;
    out vec4 color;
    layout(location = 6) in vec4 a_color;
    out float depth;
    layout(location = 7) in float a_z;
    layout(location = 2) in vec2 a_offset;
    layout(location = 3) in vec2 a_scale;
    layout(location = 0) in vec2 a_position;
    
    void main()
    {
        vec2 texcoord = a_texcoord0;
        int _19 = floatBitsToInt(a_flags);
        if ((_19 & 1) != 0)
        {
            vec2 _101 = texcoord;
            _101.x = 1.0 - _101.x;
            texcoord = _101;
        }
        if ((_19 & 2) != 0)
        {
            vec2 _104 = texcoord;
            _104.y = 1.0 - _104.y;
            texcoord = _104;
        }
        uv = texcoord * (a_uvmax - a_uvmin) + a_uvmin;
        color = a_color;
        depth = a_z;
        gl_Position = ((mat4(vs_params[8], vs_params[9], vs_params[10], vs_params[11]) * mat4(vs_params[4], vs_params[5], vs_params[6], vs_params[7])) * mat4(vs_params[0], vs_params[1], vs_params[2], vs_params[3])) * vec4(a_scale * a_position + a_offset, a_z, 1.0);
    }
    
*/
static const char sprite_vs_source_glsl300es[1125] = {
    0x23,0x76,0x65,0x72,0x73,0x69,0x6f,0x6e,0x20,0x33,0x30,0x30,0x20,0x65,0x73,0x0a,
    0x0a,0x75,0x6e,0x69,0x66,0x6f,0x72,0x6d,0x20,0x76,0x65,0x63,0x34,0x20,0x76,0x73,
    0x5f,0x70,0x61,0x72,0x61,0x6d,0x73,0x5b,0x31,0x32,0x5d,0x3b,0x0a,0x6c,0x61,0x79,
    0x6f,0x75,0x74,0x28,0x6c,0x6f,0x63,0x61,0x74,0x69,0x6f,0x6e,0x20,0x3d,0x20,0x31,
    0x29,0x20,0x69,0x6e,0x20,0x76,0x65,0x63,0x32,0x20,0x61,0x5f,0x74,0x65,0x78,0x63,
    0x6f,0x6f,0x72,0x64,0x30,0x3b,0x0a,0x6c,0x61,0x79,0x6f,0x75,0x74,0x28,0x6c,0x6f,
    0x63,0x61,0x74,0x69,0x6f,0x6e,0x20,0x3d,0x20,0x38,0x29,0x20,0x69,0x6e,0x20,0x66,
    0x6c,0x6f,0x61,0x74,0x20,0x61,0x5f,0x66,0x6c,0x61,0x67,0x73,0x3b,0x0a,0x6f,0x75,
    0x74,0x20,0x76,0x65,0x63,0x32,0x20,0x75,0x76,0x3b,0x0a,0x6c,0x61,0x79,0x6f,0x75,
    0x74,0x28,0x6c,0x6f,0x63,0x61,0x74,0x69,0x6f,0x6e,0x20,0x3d,0x20,0x34,0x29,0x20,
    0x69,0x6e,0x20,0x76,0x65,0x63,0x32,0x20,0x61,0x5f,0x75,0x76,0x6d,0x69,0x6e,0x3b,
    0x0a,0x6c,0x61,0x79,0x6f,0x75,0x74,0x28,0x6c,0x6f,0x63,0x61,0x74,0x69,0x6f,0x6e,
    0x20,0x3d,0x20,0x35,0x29,0x20,0x69,0x6e,0x20,0x76,0x65,0x63,0x32,0x20,0x61,0x5f,
    0x75,0x76,0x6d,0x61,0x78,0x3b,0x0a,0x6f,0x75,0x74,0x20,0x76,0x65,0x63,0x34,0x20,
    0x63,0x6f,0x6c,0x6f,0x72,0x3b,0x0a,0x6c,0x61,0x79,0x6f,0x75,0x74,0x28,0x6c,0x6f,
    0x63,0x61,0x74,0x69,0x6f,0x6e,0x20,0x3d,0x20,0x36,0x29,0x20,0x69,0x6e,0x20,0x76,
    0x65,0x63,0x34,0x20,0x61,0x5f,0x63,0x6f,0x6c,0x6f,0x72,0x3b,0x0a,0x6f,0x75,0x74,
    0x20,0x66,0x6c,0x6f,0x61,0x74,0x20,0x64,0x65,0x70,0x74,0x68,0x3b,0x0a,0x6c,0x61,
    0x79,0x6f,0x75,0x74,0x28,0x6c,0x6f,0x63,0x61,0x74,0x69,0x6f,0x6e,0x20,0x3d,0x20,
    0x37,0x29,0x20,0x69,0x6e,0x20,0x66,0x6c,0x6f,0x61,0x74,0x20,0x61,0x5f,0x7a,0x3b,
    0x0a,0x6c,0x61,0x79,0x6f,0x75,0x74,0x28,0x6c,0x6f,0x63,0x61,0x74,0x69,0x6f,0x6e,
    0x20,0x3d,0x20,0x32,0x29,0x20,0x69,0x6e,0x20,0x76,0x65,0x63,0x32,0x20,0x61,0x5f,
    0x6f,0x66,0x66,0x73,0x65,0x74,0x3b,0x0a,0x6c,0x61,0x79,0x6f,0x75,0x74,0x28,0x6c,
    0x6f,0x63,0x61,0x74,0x69,0x6f,0x6e,0x20,0x3d,0x20,0x33,0x29,0x20,0x69,0x6e,0x20,
    0x76,0x65,0x63,0x32,0x20,0x61,0x5f,0x73,0x63,0x61,0x6c,0x65,0x3b,0x0a,0x6c,0x61,
    0x79,0x6f,0x75,0x74,0x28,0x6c,0x6f,0x63,0x61,0x74,0x69,0x6f,0x6e,0x20,0x3d,0x20,
    0x30,0x29,0x20,0x69,0x6e,0x20,0x76,0x65,0x63,0x32,0x20,0x61,0x5f,0x70,0x6f,0x73,
    0x69,0x74,0x69,0x6f,0x6e,0x3b,0x0a,0x0a,0x76,0x6f,0x69,0x64,0x20,0x6d,0x61,0x69,
    0x6e,0x28,0x29,0x0a,0x7b,0x0a,0x20,0x20,0x20,0x20,0x76,0x65,0x63,0x32,0x20,0x74,
    0x65,0x78,0x63,0x6f,0x6f,0x72,0x64,0x20,0x3d,0x20,0x61,0x5f,0x74,0x65,0x78,0x63,
    0x6f,0x6f,0x72,0x64,0x30,0x3b,0x0a,0x20,0x20,0x20,0x20,0x69,0x6e,0x74,0x20,0x5f,
    0x31,0x39,0x20,0x3d,0x20,0x66,0x6c,0x6f,0x61,0x74,0x42,0x69,0x74,0x73,0x54,0x6f,
    0x49,0x6e,0x74,0x28,0x61,0x5f,0x66,0x6c,0x61,0x67,0x73,0x29,0x3b,0x0a,0x20,0x20,
    0x20,0x20,0x69,0x66,0x20,0x28,0x28,0x5f,0x31,0x39,0x20,0x26,0x20,0x31,0x29,0x20,
    0x21,0x3d,0x20,0x30,0x29,0x0a,0x20,0x20,0x20,0x20,0x7b,0x0a,0x20,0x20,0x20,0x20,
    0x20,0x20,0x20,0x20,0x76,0x65,0x63,0x32,0x20,0x5f,0x31,0x30,0x31,0x20,0x3d,0x20,
    0x74,0x65,0x78,0x63,0x6f,0x6f,0x72,0x64,0x3b,0x0a,0x20,0x20,0x20,0x20,0x20,0x20,
    0x20,0x20,0x5f,0x31,0x30,0x31,0x2e,0x78,0x20,0x3d,0x20,0x31,0x2e,0x30,0x20,0x2d,
    0x20,0x5f,0x31,0x30,0x31,0x2e,0x78,0x3b,0x0a,0x20,0x20,0x20,0x20,0x20,0x20,0x20,
    0x20,0x74,0x65,0x78,0x63,0x6f,0x6f,0x72,0x64,0x20,0x3d,0x20,0x5f,0x31,0x30,0x31,
    0x3b,0x0a,0x20,0x20,0x20,0x20,0x7d,0x0a,0x20,0x20,0x20,0x20,0x69,0x66,0x20,0x28,
    0x28,0x5f,0x31,0x39,0x20,0x26,0x20,0x32,0x29,0x20,0x21,0x3d,0x20,0x30,0x29,0x0a,
    0x20,0x20,0x20,0x20,0x7b,0x0a,0x20,0x20,0x20,0x20,0x20,0x20,0x20,0x20,0x76,0x65,
    0x63,0x32,0x20,0x5f,0x31,0x30,0x34,0x20,0x3d,0x20,0x74,0x65,0x78,0x63,0x6f,0x6f,
    0x72,0x64,0x3b,0x0a,0x20,0x20,0x20,0x20,0x20,0x20,0x20,0x20,0x5f,0x31,0x30,0x34,
    0x2e,0x79,0x20,0x3d,0x20,0x31,0x2e,0x30,0x20,0x2d,0x20,0x5f,0x31,0x30,0x34,0x2e,
    0x79,0x3b,0x0a,0x20,0x20,0x20,0x20,0x20,0x20,0x20,0x20,0x74,0x65,0x78,0x63,0x6f,
    0x6f,0x72,0x64,0x20,0x3d,0x20,0x5f,0x31,0x30,0x34,0x3b,0x0a,0x20,0x20,0x20,0x20,
    0x7d,0x0a,0x20,0x20,0x20,0x20,0x75,0x76,0x20,0x3d,0x20,0x74,0x65,0x78,0x63,0x6f,
    0x6f,0x72,0x64,0x20,0x2a,0x20,0x28,0x61,0x5f,0x75,0x76,0x6d,0x61,0x78,0x20,0x2d,
    0x20,0x61,0x5f,0x75,0x76,0x6d,0x69,0x6e,0x29,0x20,0x2b,0x20,0x61,0x5f,0x75,0x76,
    0x6d,0x69,0x6e,0x3b,0x0a,0x20,0x20,0x20,0x20,0x63,0x6f,0x6c,0x6f,0x72,0x20,0x3d,
    0x20,0x61,0x5f,0x63,0x6f,0x6c,0x6f,0x72,0x3b,0x0a,0x20,0x20,0x20,0x20,0x64,0x65,
    0x70,0x74,0x68,0x20,0x3d,0x20,0x61,0x5f,0x7a,0x3b,0x0a,0x20,0x20,0x20,0x20,0x67,
    0x6c,0x5f,0x50,0x6f,0x73,0x69,0x74,0x69,0x6f,0x6e,0x20,0x3d,0x20,0x28,0x28,0x6d,
    0x61,0x74,0x34,0x28,0x76,0x73,0x5f,0x70,0x61,0x72,0x61,0x6d,0x73,0x5b,0x38,0x5d,
    0x2c,0x20,0x76,0x73,0x5f,0x70,0x61,0x72,0x61,0x6d,0x73,0x5b,0x39,0x5d,0x2c,0x20,
    0x76,0x73,0x5f,0x70,0x61,0x72,0x61,0x6d,0x73,0x5b,0x31,0x30,0x5d,0x2c,0x20,0x76,
    0x73,0x5f,0x70,0x61,0x72,0x61,0x6d,0x73,0x5b,0x31,0x31,0x5d,0x29,0x20,0x2a,0x20,
    0x6d,0x61,0x74,0x34,0x28,0x76,0x73,0x5f,0x70,0x61,0x72,0x61,0x6d,0x73,0x5b,0x34,
    0x5d,0x2c,0x20,0x76,0x73,0x5f,0x70,0x61,0x72,0x61,0x6d,0x73,0x5b,0x35,0x5d,0x2c,
    0x20,0x76,0x73,0x5f,0x70,0x61,0x72,0x61,0x6d,0x73,0x5b,0x36,0x5d,0x2c,0x20,0x76,
    0x73,0x5f,0x70,0x61,0x72,0x61,0x6d,0x73,0x5b,0x37,0x5d,0x29,0x29,0x20,0x2a,0x20,
    0x6d,0x61,0x74,0x34,0x28,0x76,0x73,0x5f,0x70,0x61,0x72,0x61,0x6d,0x73,0x5b,0x30,
    0x5d,0x2c,0x20,0x76,0x73,0x5f,0x70,0x61,0x72,0x61,0x6d,0x73,0x5b,0x31,0x5d,0x2c,
    0x20,0x76,0x73,0x5f,0x70,0x61,0x72,0x61,0x6d,0x73,0x5b,0x32,0x5d,0x2c,0x20,0x76,
    0x73,0x5f,0x70,0x61,0x72,0x61,0x6d,0x73,0x5b,0x33,0x5d,0x29,0x29,0x20,0x2a,0x20,
    0x76,0x65,0x63,0x34,0x28,0x61,0x5f,0x73,0x63,0x61,0x6c,0x65,0x20,0x2a,0x20,0x61,
    0x5f,0x70,0x6f,0x73,0x69,0x74,0x69,0x6f,0x6e,0x20,0x2b,0x20,0x61,0x5f,0x6f,0x66,
    0x66,0x73,0x65,0x74,0x2c,0x20,0x61,0x5f,0x7a,0x2c,0x20,0x31,0x2e,0x30,0x29,0x3b,
    0x0a,0x7d,0x0a,0x0a,0x00,
};
/*
    #version 300 es
    precision mediump float;
    precision highp int;
    
    uniform highp sampler2D tex_smp;
    
    layout(location = 0) out highp vec4 frag_color;
    in highp vec4 color;
    in highp vec2 uv;
    in highp float depth;
    
    void main()
    {
        frag_color = color * texture(tex_smp, uv);
        if (frag_color.w < 9.9999997473787516355514526367188e-05)
        {
            discard;
        }
    }
    
*/
static const char sprite_fs_source_glsl300es[363] = {
    0x23,0x76,0x65,0x72,0x73,0x69,0x6f,0x6e,0x20,0x33,0x30,0x30,0x20,0x65,0x73,0x0a,
    0x70,0x72,0x65,0x63,0x69,0x73,0x69,0x6f,0x6e,0x20,0x6d,0x65,0x64,0x69,0x75,0x6d,
    0x70,0x20,0x66,0x6c,0x6f,0x61,0x74,0x3b,0x0a,0x70,0x72,0x65,0x63,0x69,0x73,0x69,
    0x6f,0x6e,0x20,0x68,0x69,0x67,0x68,0x70,0x20,0x69,0x6e,0x74,0x3b,0x0a,0x0a,0x75,
    0x6e,0x69,0x66,0x6f,0x72,0x6d,0x20,0x68,0x69,0x67,0x68,0x70,0x20,0x73,0x61,0x6d,
    0x70,0x6c,0x65,0x72,0x32,0x44,0x20,0x74,0x65,0x78,0x5f,0x73,0x6d,0x70,0x3b,0x0a,
    0x0a,0x6c,0x61,0x79,0x6f,0x75,0x74,0x28,0x6c,0x6f,0x63,0x61,0x74,0x69,0x6f,0x6e,
    0x20,0x3d,0x20,0x30,0x29,0x20,0x6f,0x75,0x74,0x20,0x68,0x69,0x67,0x68,0x70,0x20,
    0x76,0x65,0x63,0x34,0x20,0x66,0x72,0x61,0x67,0x5f,0x63,0x6f,0x6c,0x6f,0x72,0x3b,
    0x0a,0x69,0x6e,0x20,0x68,0x69,0x67,0x68,0x70,0x20,0x76,0x65,0x63,0x34,0x20,0x63,
    0x6f,0x6c,0x6f,0x72,0x3b,0x0a,0x69,0x6e,0x20,0x68,0x69,0x67,0x68,0x70,0x20,0x76,
    0x65,0x63,0x32,0x20,0x75,0x76,0x3b,0x0a,0x69,0x6e,0x20,0x68,0x69,0x67,0x68,0x70,
    0x20,0x66,0x6c,0x6f,0x61,0x74,0x20,0x64,0x65,0x70,0x74,0x68,0x3b,0x0a,0x0a,0x76,
    0x6f,0x69,0x64,0x20,0x6d,0x61,0x69,0x6e,0x28,0x29,0x0a,0x7b,0x0a,0x20,0x20,0x20,
    0x20,0x66,0x72,0x61,0x67,0x5f,0x63,0x6f,0x6c,0x6f,0x72,0x20,0x3d,0x20,0x63,0x6f,
    0x6c,0x6f,0x72,0x20,0x2a,0x20,0x74,0x65,0x78,0x74,0x75,0x72,0x65,0x28,0x74,0x65,
    0x78,0x5f,0x73,0x6d,0x70,0x2c,0x20,0x75,0x76,0x29,0x3b,0x0a,0x20,0x20,0x20,0x20,
    0x69,0x66,0x20,0x28,0x66,0x72,0x61,0x67,0x5f,0x63,0x6f,0x6c,0x6f,0x72,0x2e,0x77,
    0x20,0x3c,0x20,0x39,0x2e,0x39,0x39,0x39,0x39,0x39,0x39,0x37,0x34,0x37,0x33,0x37,
    0x38,0x37,0x35,0x31,0x36,0x33,0x35,0x35,0x35,0x31,0x34,0x35,0x32,0x36,0x33,0x36,
    0x37,0x31,0x38,0x38,0x65,0x2d,0x30,0x35,0x29,0x0a,0x20,0x20,0x20,0x20,0x7b,0x0a,
    0x20,0x20,0x20,0x20,0x20,0x20,0x20,0x20,0x64,0x69,0x73,0x63,0x61,0x72,0x64,0x3b,
    0x0a,0x20,0x20,0x20,0x20,0x7d,0x0a,0x7d,0x0a,0x0a,0x00,
};
#if !defined(SOKOL_GFX_INCLUDED)
  #error "Please include sokol_gfx.h before sprite.glsl.h"
#endif
static inline const sg_shader_desc* sprite_sprite_shader_desc(sg_backend backend) {
  if (backend == SG_BACKEND_GLCORE33) {
    static sg_shader_desc desc;
    static bool valid;
    if (!valid) {
      valid = true;
      desc.attrs[0].name = "a_position";
      desc.attrs[1].name = "a_texcoord0";
      desc.attrs[2].name = "a_offset";
      desc.attrs[3].name = "a_scale";
      desc.attrs[4].name = "a_uvmin";
      desc.attrs[5].name = "a_uvmax";
      desc.attrs[6].name = "a_color";
      desc.attrs[7].name = "a_z";
      desc.attrs[8].name = "a_flags";
      desc.vs.source = sprite_vs_source_glsl330;
      desc.vs.entry = "main";
      desc.vs.uniform_blocks[0].size = 192;
      desc.vs.uniform_blocks[0].layout = SG_UNIFORMLAYOUT_STD140;
      desc.vs.uniform_blocks[0].uniforms[0].name = "vs_params";
      desc.vs.uniform_blocks[0].uniforms[0].type = SG_UNIFORMTYPE_FLOAT4;
      desc.vs.uniform_blocks[0].uniforms[0].array_count = 12;
      desc.fs.source = sprite_fs_source_glsl330;
      desc.fs.entry = "main";
      desc.fs.images[0].used = true;
      desc.fs.images[0].multisampled = false;
      desc.fs.images[0].image_type = SG_IMAGETYPE_2D;
      desc.fs.images[0].sample_type = SG_IMAGESAMPLETYPE_FLOAT;
      desc.fs.samplers[0].used = true;
      desc.fs.samplers[0].sampler_type = SG_SAMPLERTYPE_FILTERING;
      desc.fs.image_sampler_pairs[0].used = true;
      desc.fs.image_sampler_pairs[0].image_slot = 0;
      desc.fs.image_sampler_pairs[0].sampler_slot = 0;
      desc.fs.image_sampler_pairs[0].glsl_name = "tex_smp";
      desc.label = "sprite_sprite_shader";
    }
    return &desc;
  }
  if (backend == SG_BACKEND_GLES3) {
    static sg_shader_desc desc;
    static bool valid;
    if (!valid) {
      valid = true;
      desc.attrs[0].name = "a_position";
      desc.attrs[1].name = "a_texcoord0";
      desc.attrs[2].name = "a_offset";
      desc.attrs[3].name = "a_scale";
      desc.attrs[4].name = "a_uvmin";
      desc.attrs[5].name = "a_uvmax";
      desc.attrs[6].name = "a_color";
      desc.attrs[7].name = "a_z";
      desc.attrs[8].name = "a_flags";
      desc.vs.source = sprite_vs_source_glsl300es;
      desc.vs.entry = "main";
      desc.vs.uniform_blocks[0].size = 192;
      desc.vs.uniform_blocks[0].layout = SG_UNIFORMLAYOUT_STD140;
      desc.vs.uniform_blocks[0].uniforms[0].name = "vs_params";
      desc.vs.uniform_blocks[0].uniforms[0].type = SG_UNIFORMTYPE_FLOAT4;
      desc.vs.uniform_blocks[0].uniforms[0].array_count = 12;
      desc.fs.source = sprite_fs_source_glsl300es;
      desc.fs.entry = "main";
      desc.fs.images[0].used = true;
      desc.fs.images[0].multisampled = false;
      desc.fs.images[0].image_type = SG_IMAGETYPE_2D;
      desc.fs.images[0].sample_type = SG_IMAGESAMPLETYPE_FLOAT;
      desc.fs.samplers[0].used = true;
      desc.fs.samplers[0].sampler_type = SG_SAMPLERTYPE_FILTERING;
      desc.fs.image_sampler_pairs[0].used = true;
      desc.fs.image_sampler_pairs[0].image_slot = 0;
      desc.fs.image_sampler_pairs[0].sampler_slot = 0;
      desc.fs.image_sampler_pairs[0].glsl_name = "tex_smp";
      desc.label = "sprite_sprite_shader";
    }
    return &desc;
  }
  return 0;
}
