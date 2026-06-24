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
    vec4 color = fs_in.color;

    // Static materials need to read as a surface. Circular sprites expose the
    // particle lattice, so material particles use a lightly feathered square.
    if (fs_in.type >= 10u) {
        vec2 edge = min(fs_in.uv, 1.0 - fs_in.uv);
        float feather = smoothstep(0.0, 0.10, min(edge.x, edge.y));
        color.a *= max(feather, 0.82);
        fragColor = vec4(color.rgb * color.a, color.a);
        bloomColor = vec4(0.0);
        return;
    }

    vec2 coord = fs_in.uv * 2.0 - 1.0;
    float rSq = dot(coord, coord);
    if (rSq > 1.0) {
        discard;
    }

    float r = sqrt(rSq);
    float alpha = 1.0 - r;

    if (fs_in.type == 1u) { // FIRE
        float core = smoothstep(0.70, 0.0, r);
        float glow = smoothstep(1.0, 0.16, r);
        color.rgb = mix(color.rgb * 0.70, vec3(1.35, 0.82, 0.24), core);
        alpha = glow * 0.72 + core * 0.28;
    } else if (fs_in.type == 3u) { // SMOKE
        alpha = pow(alpha, 1.8) * 0.36;
        color.rgb *= 0.75;
    } else if (fs_in.type == 5u) { // SPARK
        alpha = pow(alpha, 3.2);
    } else if (fs_in.type == 4u) { // ASH
        alpha = pow(alpha, 1.4) * 0.45;
    } else {
        alpha = pow(alpha, 1.2);
    }

    color.a *= alpha;

    if (useSoftParticles) {
        vec2 screenTexCoord = gl_FragCoord.xy / screenSize;
        float depth = texture(depthMap, screenTexCoord).r;
        float fade = clamp((depth - gl_FragCoord.z) * softParticleScale, 0.0, 1.0);
        color.a *= fade;
    }

    fragColor = vec4(color.rgb * color.a, color.a);

    float brightness = dot(color.rgb, vec3(0.2126, 0.7152, 0.0722));
    float bloomMask = smoothstep(0.55, 1.15, brightness) * color.a;
    bloomColor = vec4(color.rgb * bloomMask, bloomMask);
}
