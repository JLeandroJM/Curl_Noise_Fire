#version 430 core

out vec4 FragColor;
in vec2 TexCoords;

uniform sampler2D scene;
uniform float threshold = 1.0;

void main() {
    // Leer el color base de la escena renderizada
    vec4 color = texture(scene, TexCoords);
    
    // Extraer luminancia basándose en los pesos de percepción estándar (luma REC 709)
    float brightness = dot(color.rgb, vec3(0.2126, 0.7152, 0.0722));
    
    // Si la intensidad de brillo supera el umbral, se escribe al buffer, sino negro
    if (brightness > threshold) {
        FragColor = vec4(color.rgb, 1.0);
    } else {
        FragColor = vec4(0.0, 0.0, 0.0, 1.0);
    }
}
