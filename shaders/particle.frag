#version 430 core

in FragData {
    vec2 uv;
    vec4 color;
    flat uint type;
} fs_in;

// Dos salidas (MRT - Multiple Render Targets)
layout(location = 0) out vec4 fragColor;  // Color principal
layout(location = 1) out vec4 bloomColor; // Color brillante a extraer para Bloom

// Texture Depth Map para hacer "Soft Particles"
uniform sampler2D depthMap;
uniform vec2 screenSize;
uniform float softParticleScale = 1.0;
uniform bool useSoftParticles = false;

void main() {
    // Trasladar UV [0,1] al rango local circular [-1,1]
    vec2 coord = fs_in.uv * 2.0 - 1.0;
    float rSq = dot(coord, coord); // Distancia al cuadrado (x^2 + y^2)
    
    // Descartar pixeles fuera del círculo
    if (rSq > 1.0) discard;

    // Atenuación radial suave (alpha base)
    float alpha = 1.0 - sqrt(rSq);

    // Ajustar el perfil (forma) de la atenuación según el tipo de partícula
    if (fs_in.type == 1u) { // FIRE
        alpha = pow(alpha, 1.5); // Centro brillante, bordes suaves rápidos
    } else if (fs_in.type == 3u) { // SMOKE
        alpha = pow(alpha, 0.8); // Mucho más difuso y redondo
    } else if (fs_in.type == 5u) { // SPARK
        alpha = pow(alpha, 3.0); // Muy puntual e intenso en el centro
    } else if (fs_in.type >= 10u) { // Material estático
        alpha = pow(alpha, 0.5); // Bloques duros
    }

    vec4 color = fs_in.color;
    color.a *= alpha;

    // Evaluación de Soft Particles para no cortar con geometría dura
    if (useSoftParticles) {
        vec2 screenTexCoord = gl_FragCoord.xy / screenSize;
        float depth = texture(depthMap, screenTexCoord).r;
        
        // z real asumiendo lineal o calculo simple
        float z = gl_FragCoord.z; 
        float fade = clamp((depth - z) * softParticleScale, 0.0, 1.0);
        color.a *= fade;
    }

    // Premultiplied Alpha para mezcla aditiva correcta en el framebuffer
    fragColor = vec4(color.rgb * color.a, color.a);
    
    // Extraer HDR brillante para el Bloom (> umbral 1.0)
    // Calcula luminancia perceptual
    float brightness = dot(color.rgb, vec3(0.2126, 0.7152, 0.0722));
    if (brightness > 1.0) {
        bloomColor = fragColor;
    } else {
        bloomColor = vec4(0.0, 0.0, 0.0, 0.0);
    }
}
