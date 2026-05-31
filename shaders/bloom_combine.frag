#version 430 core

out vec4 FragColor;
in vec2 TexCoords;

uniform sampler2D scene;      // Textura base renderizada (HDR)
uniform sampler2D bloomBlur;  // Textura post-desenfoque
uniform float bloomIntensity = 1.0;

void main() {
    // Cargar colores de ambos buffers
    vec3 hdrColor = texture(scene, TexCoords).rgb;      
    vec3 bloomColor = texture(bloomBlur, TexCoords).rgb;
    
    // Añadir el efecto de resplandor (Bloom aditivo)
    hdrColor += bloomColor * bloomIntensity;
    
    // Mapeo de Tonos (Tone Mapping) usando Reinhard para pasar HDR a LDR (0.0 a 1.0)
    // Esto asegura que los colores super brillantes no se recorten abruptamente a blanco puro
    vec3 result = hdrColor / (hdrColor + vec3(1.0));
    
    // Corrección Gamma estándar (2.2) para el espacio de color sRGB del monitor
    const float gamma = 2.2;
    result = pow(result, vec3(1.0 / gamma));
    
    FragColor = vec4(result, 1.0);
}
