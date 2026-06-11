#version 430 core

out vec4 FragColor;
in vec2 TexCoords;

uniform sampler2D image;
uniform bool horizontal;

// Pesos para filtro Gaussiano de 13 taps (núcleo precalculado)
// Representan un lado del sombrero Gaussiano incluyendo el centro en weight[0]
uniform float weight[7] = float[] (0.196482, 0.155364, 0.075488, 0.022513, 0.004128, 0.000465, 0.000032);

void main() {
    // Calcular el tamaño de un único téxel para desplazamientos
    vec2 tex_offset = 1.0 / textureSize(image, 0); 
    
    // Contribución del píxel central
    vec3 result = texture(image, TexCoords).rgb * weight[0]; 

    // Desenfoque separable Gaussiano (dos pasadas: horizontal y vertical)
    if (horizontal) {
        for (int i = 1; i < 7; ++i) {
            result += texture(image, TexCoords + vec2(tex_offset.x * float(i), 0.0)).rgb * weight[i];
            result += texture(image, TexCoords - vec2(tex_offset.x * float(i), 0.0)).rgb * weight[i];
        }
    } else {
        for (int i = 1; i < 7; ++i) {
            result += texture(image, TexCoords + vec2(0.0, tex_offset.y * float(i))).rgb * weight[i];
            result += texture(image, TexCoords - vec2(0.0, tex_offset.y * float(i))).rgb * weight[i];
        }
    }
    
    FragColor = vec4(result, 1.0);
}
