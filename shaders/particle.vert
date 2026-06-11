#version 430 core

// CONSTANTES DE TIPOS DE PARTÍCULA
#define TYPE_DEAD     0u
#define TYPE_FIRE     1u
#define TYPE_EMBER    2u
#define TYPE_SMOKE    3u
#define TYPE_ASH      4u
#define TYPE_SPARK    5u
#define TYPE_PAPER    10u
#define TYPE_CEMENT   11u
#define TYPE_WOOD     12u
#define TYPE_LEAF     13u
#define TYPE_GROUND   14u

// Estructura de Partícula
struct Particle {
    vec4 position;      // xyz = posición, w = tamaño
    vec4 velocity;      // xyz = velocidad, w = masa
    vec4 color;         // rgba
    float lifetime;     // tiempo de vida restante
    float maxLifetime;  // tiempo de vida total
    float temperature;  // nivel de calor
    uint type;          // tipo de partícula
};

// Lectura de SSBO
layout(std430, binding = 0) buffer ParticleBuffer {
    Particle particles[];
};

// Salidas hacia el Geometry Shader
out VertexData {
    vec4 color;
    float size;
    uint type;
} vs_out;

void main() {
    // Obtenemos los datos de la partícula basados en el índice del vértice
    Particle p = particles[gl_VertexID];
    
    // Asignar posición original en mundo
    gl_Position = vec4(p.position.xyz, 1.0);
    
    // Pasar los atributos relevantes
    vs_out.color = p.color;
    vs_out.size = p.position.w;
    vs_out.type = p.type;
}
