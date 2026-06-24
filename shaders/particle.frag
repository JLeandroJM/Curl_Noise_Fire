#version 430 core

in FragData {
    vec2 uv;
    vec4 color;
    flat uint type;
} fs_in;

layout(location = 0) out vec4 fragColor;
layout(location = 1) out vec4 bloomColor;

uniform sampler2D depthMap;
uniform vec2 screenSize;
uniform float softParticleScale = 1.0;
uniform bool useSoftParticles = false;

void main() {
    vec2 coord = fs_in.uv * 2.0 - 1.0;
    float rSq = dot(coord, coord);

    bool isMaterial = fs_in.type >= 10u;
    if (rSq > 1.0) {
        discard;
    }

    // Material: nucleo opaco con borde redondo suave (evita el look cuadriculado).
    float alpha = isMaterial ? smoothstep(1.0, 0.7, rSq) : 1.0 - sqrt(rSq);

    if (fs_in.type == 1u) {
        alpha = pow(alpha, 1.55);
    } else if (fs_in.type == 3u) {
        alpha = pow(alpha, 1.9) * 0.34;
    } else if (fs_in.type == 4u) {
        alpha = pow(alpha, 2.8) * 0.45;
    } else if (fs_in.type == 5u) {
        alpha = pow(alpha, 3.0);
    }

    vec4 color = fs_in.color;
    color.a *= alpha;

    if (useSoftParticles) {
        vec2 screenTexCoord = gl_FragCoord.xy / screenSize;
        float depth = texture(depthMap, screenTexCoord).r;
        float z = gl_FragCoord.z;
        float fade = clamp((depth - z) * softParticleScale, 0.0, 1.0);
        color.a *= fade;
    }

    if (fs_in.type == 1u) {
        color.rgb *= 1.18;
    } else if (fs_in.type == 5u) {
        color.rgb *= 1.45;
    }

    fragColor = color;

    float brightness = dot(color.rgb, vec3(0.2126, 0.7152, 0.0722));
    bloomColor = brightness > 1.0 ? fragColor : vec4(0.0);
}
