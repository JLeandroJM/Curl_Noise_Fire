#version 430 core

in PaperData {
    vec4 color;
    float burnStart;
    vec2 uv;
    vec3 worldPos;
} fs_in;

layout(location = 0) out vec4 fragColor;
layout(location = 1) out vec4 bloomColor;

uniform float u_time;
uniform float u_burnDuration;

float hash21(vec2 p) {
    return fract(sin(dot(p, vec2(127.1, 311.7))) * 43758.5453123);
}

float fiber(vec2 uv) {
    float f1 = sin(uv.x * 520.0 + uv.y * 38.0) * 0.018;
    float f2 = sin(uv.y * 360.0 + uv.x * 22.0) * 0.014;
    float f3 = hash21(floor(uv * 80.0)) * 0.025;
    return f1 + f2 + f3;
}

void main() {
    float phase = clamp((u_time - fs_in.burnStart) / u_burnDuration, -1.0, 1.25);
    float grain = hash21(fs_in.uv * 90.0 + fs_in.burnStart);

    vec3 paper = fs_in.color.rgb + vec3(fiber(fs_in.uv));
    vec3 warm = vec3(0.78, 0.54, 0.28);
    vec3 scorch = vec3(0.30, 0.18, 0.08);
    vec3 charred = vec3(0.018, 0.016, 0.014);
    vec3 ash = vec3(0.15, 0.145, 0.135);

    vec3 color = paper;

    if (phase > 0.0) {
        float heat = smoothstep(0.0, 0.13, phase);
        float charPhase = smoothstep(0.10, 0.48, phase + (grain - 0.5) * 0.10);
        float ashPhase = smoothstep(0.45, 0.86, phase + (grain - 0.5) * 0.14);

        color = mix(color, warm, heat * 0.45);
        color = mix(color, scorch, charPhase);
        color = mix(color, charred, smoothstep(0.22, 0.70, phase));
        color = mix(color, ash, ashPhase * 0.55);

        float rim = smoothstep(0.05, 0.16, phase) * (1.0 - smoothstep(0.17, 0.30, phase));
        vec3 ember = mix(vec3(1.0, 0.28, 0.03), vec3(1.0, 0.82, 0.22), grain);
        color = mix(color, ember, rim * 0.65);
    }

    float holeNoise = hash21(fs_in.uv * 140.0 + vec2(fs_in.burnStart, 2.3));
    float crumble = smoothstep(0.76, 1.0, phase + (holeNoise - 0.5) * 0.28);
    if (crumble > 0.62) {
        discard;
    }

    fragColor = vec4(color, 1.0);

    float emberGlow = smoothstep(0.04, 0.14, phase) * (1.0 - smoothstep(0.16, 0.30, phase));
    bloomColor = vec4(color * emberGlow * 0.55, emberGlow);
}
