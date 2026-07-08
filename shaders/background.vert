#version 430 core

out vec2 vUV;
out vec2 TexCoords;
out vec2 TexCoord;
out vec2 vTexCoord;

const vec2 positions[3] = vec2[3](
    vec2(-1.0, -1.0),
    vec2( 3.0, -1.0),
    vec2(-1.0,  3.0)
);

void main()
{
    vec2 pos = positions[gl_VertexID];

    gl_Position = vec4(pos, 0.0, 1.0);

    vec2 uv = pos * 0.5 + 0.5;

    vUV = uv;
    TexCoords = uv;
    TexCoord = uv;
    vTexCoord = uv;
}