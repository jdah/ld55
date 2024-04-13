@module screenquad

@vs vs
in vec2 position;
in vec2 texcoord0;

uniform vs_params {
	mat4 model;
    mat4 view;
    mat4 proj;
};

out vec2 uv;
void main() {
    gl_Position = proj * model * view * vec4(position, 0.0, 1.0);
    uv = texcoord0;
}
@end

@fs fs
uniform texture2D tex;
uniform sampler smp;

in vec2 uv;
out vec4 frag_color;
void main() {
    frag_color = texture(sampler2D(tex, smp), uv);
}
@end

@program screenquad vs fs
