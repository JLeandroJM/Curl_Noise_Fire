#version 430 core

layout(points) in;
layout(triangle_strip, max_vertices = 4) out;

in VertexData {
    vec4 color;
    float size;
    uint type;
} gs_in[];

out FragData {
    vec2 uv;
    vec4 color;
    flat uint type;
} gs_out;

uniform mat4 view;
uniform mat4 projection;
uniform vec3 cameraRight;
uniform vec3 cameraUp;
uniform int renderPass;

void main() {
    if (gs_in[0].type == 0u) {
        return;
    }

    bool isMaterial = gs_in[0].type >= 10u;
    if ((renderPass == 0 && !isMaterial) || (renderPass == 1 && isMaterial)) {
        return;
    }

    vec3 pos = gl_in[0].gl_Position.xyz;
    float size = gs_in[0].size;

    if (gs_in[0].type == 3u) {
        size *= 0.55;
    } else if (gs_in[0].type == 4u) {
        size *= 0.32;
    } else if (gs_in[0].type == 5u) {
        size *= 0.22;
    } else if (gs_in[0].type >= 10u) {
        size *= 1.18;
    }

    vec3 right = cameraRight * size;
    vec3 up = cameraUp * size;

    gl_Position = projection * view * vec4(pos + right + up, 1.0);
    gs_out.uv = vec2(1.0, 1.0);
    gs_out.color = gs_in[0].color;
    gs_out.type = gs_in[0].type;
    EmitVertex();

    gl_Position = projection * view * vec4(pos - right + up, 1.0);
    gs_out.uv = vec2(0.0, 1.0);
    gs_out.color = gs_in[0].color;
    gs_out.type = gs_in[0].type;
    EmitVertex();

    gl_Position = projection * view * vec4(pos + right - up, 1.0);
    gs_out.uv = vec2(1.0, 0.0);
    gs_out.color = gs_in[0].color;
    gs_out.type = gs_in[0].type;
    EmitVertex();

    gl_Position = projection * view * vec4(pos - right - up, 1.0);
    gs_out.uv = vec2(0.0, 0.0);
    gs_out.color = gs_in[0].color;
    gs_out.type = gs_in[0].type;
    EmitVertex();

    EndPrimitive();
}
