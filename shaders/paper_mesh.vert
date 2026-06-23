#version 430 core

layout(location = 0) in vec3 a_position;
layout(location = 1) in vec4 a_color;
layout(location = 2) in float a_burnStart;
layout(location = 3) in vec2 a_uv;

out PaperData {
    vec4 color;
    float burnStart;
    vec2 uv;
    vec3 worldPos;
} vs_out;

uniform mat4 u_view;
uniform mat4 u_projection;
uniform float u_time;

void main() {
    float wave = sin(a_position.x * 5.0 + u_time * 1.7) *
                 cos(a_position.z * 4.0 + u_time * 1.1);

    vec3 pos = a_position;
    pos.y += wave * 0.006;

    vs_out.color = a_color;
    vs_out.burnStart = a_burnStart;
    vs_out.uv = a_uv;
    vs_out.worldPos = pos;

    gl_Position = u_projection * u_view * vec4(pos, 1.0);
}
