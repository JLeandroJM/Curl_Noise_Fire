#version 430 core

in FragData {
    vec2 uv;
    vec4 color;
    float paperFlame;
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
        // Núcleo brillante + halo muy suave. Alpha bajo para que las
        // partículas se acumulen aditivamente y formen una masa continua
        // en vez de discos separados.
        if (fs_in.paperFlame > 0.5) {
            float y = fs_in.uv.y;
            float x = abs(coord.x);
            float edgeNoise = sin(x * 27.0 + y * 33.0 + color.g * 11.0) * 0.5
                            + sin(x * 55.0 - y * 17.0 + color.r * 7.0) * 0.5;
            float width = 0.76 + edgeNoise * 0.08;
            float crawl = 1.0 - smoothstep(width * 0.35, width, x);
            float lift = smoothstep(0.03, 0.22, y) * (1.0 - smoothstep(0.58, 0.92, y));
            float hotCenter = (1.0 - smoothstep(0.0, 0.20, x)) * smoothstep(0.10, 0.34, y) * (1.0 - smoothstep(0.46, 0.78, y));
            float smokeCut = smoothstep(0.78, 1.0, y) * (0.20 + 0.16 * edgeNoise);
            color.rgb = mix(color.rgb * 0.90, vec3(1.25, 0.74, 0.20), hotCenter);
            alpha = max(0.0, crawl * lift * 0.26 + hotCenter * 0.18 - smokeCut);
        } else {
            float y = fs_in.uv.y;
            float x = abs(coord.x);
            float width = mix(0.95, 0.18, smoothstep(0.12, 1.0, y));
            float tongue = 1.0 - smoothstep(width * 0.35, width, x);
            float base = smoothstep(0.0, 0.22, y);
            float tipFade = 1.0 - smoothstep(0.82, 1.0, y);
            float core = tongue * base * tipFade;
            float glow = (1.0 - smoothstep(0.0, width * 1.25, x)) * base * (1.0 - smoothstep(0.94, 1.0, y));
            float hotCenter = (1.0 - smoothstep(0.0, width * 0.30, x)) * smoothstep(0.08, 0.36, y) * tipFade;
            color.rgb = mix(color.rgb * 0.92, vec3(1.35, 0.86, 0.28), hotCenter);
            alpha = glow * 0.20 + core * 0.36 + hotCenter * 0.22;
        }
    } else if (fs_in.type == 3u) { // SMOKE
        alpha = pow(alpha, 2.2) * 0.16;
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
