#version 430 core

out vec4 FragColor;
in vec2 TexCoords;

// Textura de fondo (por ejemplo: frame de cámara procesado vía OpenCV)
uniform sampler2D backgroundTexture;

void main() {
    // Muestrear el fondo sin alteraciones
    vec3 color = texture(backgroundTexture, TexCoords).rgb;
    
    // Retornar en espacio HDR/LDR base a la espera del Tone Mapping final si procede.
    FragColor = vec4(color, 1.0);
}
