#pragma once
// =============================================================================
// Particle.h - Estructura de datos de partícula (GPU-compatible, std430)
// Proyecto: Simulación de Fuego con Curl Noise
// Universidad: UTEC
// =============================================================================

#include <glm/glm.hpp>
#include <cstdint>
#include <vector>
// ---------------------------------------------------------------------------
// Tipos de partícula — deben coincidir con los #define en los shaders GLSL
// ---------------------------------------------------------------------------
enum ParticleType : uint32_t {
    TYPE_DEAD     = 0u,   // Partícula muerta / disponible para reciclar
    TYPE_FIRE     = 1u,   // Llama activa
    TYPE_EMBER    = 2u,   // Brasa incandescente
    TYPE_SMOKE    = 3u,   // Humo
    TYPE_ASH      = 4u,   // Ceniza
    TYPE_SPARK    = 5u,   // Chispa
    // Materiales (partículas estáticas que forman objetos)
    TYPE_PAPER    = 10u,  // Papel
    TYPE_CEMENT   = 11u,  // Cemento / concreto
    TYPE_WOOD     = 12u,  // Madera
    TYPE_LEAF     = 13u,  // Hoja de árbol
    TYPE_GROUND   = 14u,  // Suelo
    TYPE_CHAR     = 15u,  // Residuo carbonizado
    TYPE_FABRIC   = 16u,  // Tela / cortina / cama
    TYPE_CARPET   = 17u,  // Alfombra
    TYPE_WALL     = 18u,  // Pared / yeso
    TYPE_METAL_GLASS = 19u, // Metal / vidrio
};

// ---------------------------------------------------------------------------
// Estructura de partícula — 64 bytes, alineada a std430 para SSBO
// IMPORTANTE: Esta estructura debe ser idéntica en C++ y GLSL
// ---------------------------------------------------------------------------
struct Particle {
    glm::vec4 position;     // xyz = posición mundial, w = tamaño visual
    glm::vec4 velocity;     // xyz = velocidad, w = masa (o burnStartTime para materiales)
    glm::vec4 color;        // rgba color actual
    float     lifetime;     // Tiempo de vida restante (segundos)
    float     maxLifetime;  // Tiempo de vida total (segundos)
    float     temperature;  // Temperatura normalizada [0, 1]
    uint32_t  type;         // ParticleType
};
// static_assert(sizeof(Particle) == 64, "Particle must be 64 bytes for GPU alignment");

// ---------------------------------------------------------------------------
// Parámetros de Curl Noise — enviados como uniforms al compute shader
// ---------------------------------------------------------------------------
struct CurlNoiseParams {
    float frequency    = 1.5f;   // Frecuencia base del simplex noise
    float amplitude    = 2.0f;   // Amplitud de la turbulencia
    int   octaves      = 4;      // Número de octavas (fractal Brownian motion)
    float lacunarity   = 2.0f;   // Factor multiplicador de frecuencia entre octavas
    float persistence  = 0.5f;   // Factor multiplicador de amplitud entre octavas
    float timeScale    = 0.8f;   // Velocidad de evolución temporal del noise
    float epsilon      = 0.01f;  // Paso para diferencias finitas del curl
    float boundaryWidth = 0.5f;  // Ancho de la zona de transición cerca de obstáculos
};

// ---------------------------------------------------------------------------
// Configuración de emisor — describe dónde y cómo nacen partículas de fuego
// ---------------------------------------------------------------------------
enum class EmitterShape : uint32_t {
    POINT     = 0,   // Fuente puntual
    DISK      = 1,   // Disco circular
    RECTANGLE = 2,   // Rectángulo (papel, pared)
    LINE      = 3,   // Línea (borde)
    SPHERE    = 4,   // Superficie esférica
};

struct EmitterConfig {
    glm::vec3  position      = glm::vec3(0.0f);
    glm::vec3  direction     = glm::vec3(0.0f, 1.0f, 0.0f); // Dirección preferida
    EmitterShape shape       = EmitterShape::DISK;
    float      width         = 0.5f;    // Para RECTANGLE
    float      height        = 0.5f;    // Para RECTANGLE
    float      radius        = 0.3f;    // Para DISK, SPHERE
    float      emitRate      = 5000.0f; // Partículas por segundo
    float      particleLife  = 2.0f;    // Vida base (segundos)
    float      lifeVariance  = 0.5f;    // Variación de vida
    float      initialSpeed  = 1.5f;    // Velocidad inicial
    float      speedVariance = 0.5f;    // Variación de velocidad
    float      temperature   = 1.0f;    // Temperatura inicial
    float      particleSize  = 0.05f;   // Tamaño base
    bool       active        = true;

    // Ventana de actividad en el tiempo de escena (segundos).
    // Permite que distintos emisores ardan en momentos distintos
    // (p.ej. copa del árbol primero, tronco después).
    float      startTime     = 0.0f;
    float      endTime       = 1.0e9f;
    float      fadeIn        = 0.5f;
    float      fadeOut       = 1.0f;
};

// ---------------------------------------------------------------------------
// GPU Emitter struct — versión simplificada para enviar al compute shader
// Debe coincidir con el struct en particle_emit.comp
// ---------------------------------------------------------------------------
struct GPUEmitter {
    glm::vec4 positionAndShape;   // xyz = position, w = shape (as float)
    glm::vec4 directionAndRate;   // xyz = direction, w = emitRate
    glm::vec4 dimensions;        // x = width, y = height, z = radius, w = particleLife
    glm::vec4 speedAndTemp;      // x = initialSpeed, y = speedVariance, z = temperature, w = particleSize
    glm::vec4 timing;            // x = startTime, y = endTime, z = fadeIn, w = fadeOut
};

// ---------------------------------------------------------------------------
// Punto de cámara para animación pre-definida
// ---------------------------------------------------------------------------
struct CameraKeyframe {
    float time;           // Tiempo en la animación (segundos)
    glm::vec3 position;   // Posición de la cámara
    glm::vec3 target;     // Punto al que mira
};

// ---------------------------------------------------------------------------
// Ruta de cámara para una escena
// ---------------------------------------------------------------------------
struct CameraPath {
    std::vector<CameraKeyframe> keyframes;
    float totalDuration = 10.0f;
};
