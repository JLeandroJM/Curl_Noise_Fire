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
    float paperFlame;
    flat uint type;
} gs_out;

uniform mat4 view;
uniform mat4 projection;
uniform vec3 cameraRight;
uniform vec3 cameraUp;

void main() {
    if (gs_in[0].type == 0u) {
        return;
    }

    vec3 pos = gl_in[0].gl_Position.xyz;
    float size = gs_in[0].size;
    float shapeNoise = fract(sin(dot(pos.xz, vec2(91.17, 37.43))) * 43758.5453);
    bool paperFlame = gs_in[0].type == 10u
        && gs_in[0].color.r > 0.78
        && gs_in[0].color.g > 0.22
        && gs_in[0].color.b < 0.28;

    if (gs_in[0].type == 1u) { // FIRE
        size *= 1.7;
    } else if (paperFlame) {
        size *= mix(2.15, 3.05, shapeNoise);
    } else if (gs_in[0].type == 3u) { // SMOKE
        size *= 2.0;
    } else if (gs_in[0].type == 4u) { // ASH
        size *= 0.7;
    } else if (gs_in[0].type == 5u) { // SPARK
        size *= 0.22;
    } else if (gs_in[0].type >= 10u) { // STATIC MATERIALS
        size *= 0.92;
    }

    vec3 right = cameraRight * size;
    vec3 up = cameraUp * size;
    if (paperFlame) {
        right *= mix(1.32, 1.82, shapeNoise);
        up *= mix(0.32, 0.56, shapeNoise);
        pos += cameraRight * size * (shapeNoise - 0.5) * 0.70;
        pos += cameraUp * size * mix(0.04, 0.13, shapeNoise);
    }

    gl_Position = projection * view * vec4(pos + right + up, 1.0);
    gs_out.uv = vec2(1.0, 1.0);
    gs_out.color = gs_in[0].color;
    gs_out.paperFlame = paperFlame ? 1.0 : 0.0;
    gs_out.type = paperFlame ? 1u : gs_in[0].type;
    EmitVertex();

    gl_Position = projection * view * vec4(pos - right + up, 1.0);
    gs_out.uv = vec2(0.0, 1.0);
    gs_out.color = gs_in[0].color;
    gs_out.paperFlame = paperFlame ? 1.0 : 0.0;
    gs_out.type = paperFlame ? 1u : gs_in[0].type;
    EmitVertex();

    gl_Position = projection * view * vec4(pos + right - up, 1.0);
    gs_out.uv = vec2(1.0, 0.0);
    gs_out.color = gs_in[0].color;
    gs_out.paperFlame = paperFlame ? 1.0 : 0.0;
    gs_out.type = paperFlame ? 1u : gs_in[0].type;
    EmitVertex();

    gl_Position = projection * view * vec4(pos - right - up, 1.0);
    gs_out.uv = vec2(0.0, 0.0);
    gs_out.color = gs_in[0].color;
    gs_out.paperFlame = paperFlame ? 1.0 : 0.0;
    gs_out.type = paperFlame ? 1u : gs_in[0].type;
    EmitVertex();

    EndPrimitive();
}
