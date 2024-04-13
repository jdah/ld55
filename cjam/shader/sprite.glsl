@module sprite

@vs vs
in vec2 a_position;
in vec2 a_texcoord0;

in vec2 a_offset;
in vec2 a_scale;
in vec2 a_uvmin;
in vec2 a_uvmax;
in vec4 a_color;
in float a_z;
in float a_flags;

uniform vs_params {
	mat4 model;
    mat4 view;
    mat4 proj;
};

out vec2 uv;
out vec4 color;
out float depth;

// must match with util/sprite.h
#define SPRITE_FLIP_X     (1 << 0)
#define SPRITE_FLIP_Y     (1 << 1)
#define SPRITE_ROTATE_CW  (1 << 2)
#define SPRITE_ROTATE_CCW (1 << 3)

void main() {
    vec2 texcoord = a_texcoord0;
    const int flags = floatBitsToInt(a_flags);

    if ((flags & SPRITE_FLIP_X) != 0) {
        texcoord.x = 1.0 - texcoord.x;
    }

    if ((flags & SPRITE_FLIP_Y) != 0) {
        texcoord.y = 1.0 - texcoord.y;
    }

    uv = a_uvmin + (texcoord * (a_uvmax - a_uvmin));
    color = a_color;
    depth = a_z;

    const vec2 ipos = a_offset + (a_scale * a_position);
    gl_Position = proj * view * model * vec4(ipos, a_z, 1.0);
}
@end

@fs fs
uniform texture2D tex;
uniform sampler smp;

in vec2 uv;
in vec4 color;
in float depth;

out vec4 frag_color;

void main() {
    frag_color = color * texture(sampler2D(tex, smp), uv);
    if (frag_color.a < 0.0001) {
        discard;
    }
    /* gl_FragDepth = depth; */
}
@end

@program sprite vs fs
