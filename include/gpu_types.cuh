// =============================================================================
// gpu_types.cuh - Estructuras GPU (device) con layout binario idéntico al host.
// GpuParticle debe ser byte-compatible con `struct Particle` (include/Particle.h)
// para poder hacer cudaMemcpy directo del std::vector<Particle>.
// =============================================================================
#pragma once

#include <cuda_runtime.h>
#include <cstdint>

// Tipos de partícula — deben coincidir con los #define de los shaders y Particle.h
#define G_TYPE_DEAD     0u
#define G_TYPE_FIRE     1u
#define G_TYPE_EMBER    2u
#define G_TYPE_SMOKE    3u
#define G_TYPE_ASH      4u
#define G_TYPE_SPARK    5u
#define G_TYPE_PAPER    10u
#define G_TYPE_CEMENT   11u
#define G_TYPE_WOOD     12u
#define G_TYPE_LEAF     13u
#define G_TYPE_GROUND   14u

// Formas de emisor — coinciden con EmitterShape (Particle.h) y particle_emit.comp
#define G_SHAPE_POINT     0u
#define G_SHAPE_DISK      1u
#define G_SHAPE_RECTANGLE 2u
#define G_SHAPE_LINE      3u
#define G_SHAPE_SPHERE    4u

// 64 bytes. Mismo orden que `struct Particle` en el host (3x vec4 + 3 float + uint).
struct GpuParticle {
    float4   position;     // xyz = posición, w = tamaño
    float4   velocity;     // xyz = velocidad, w = masa / burnStartTime
    float4   color;        // rgba
    float    lifetime;
    float    maxLifetime;
    float    temperature;
    uint32_t type;
};
static_assert(sizeof(GpuParticle) == 64, "GpuParticle debe ser de 64 bytes (igual que Particle en host)");

// Espejo de GPUEmitter (Particle.h): 4x vec4 = 64 bytes.
struct GpuEmitter {
    float4 positionAndShape;   // xyz = position, w = shape
    float4 directionAndRate;   // xyz = direction, w = emitRate
    float4 dimensions;         // x = width, y = height, z = radius, w = particleLife
    float4 speedAndTemp;       // x = initialSpeed, y = speedVariance, z = temperature, w = particleSize
};
static_assert(sizeof(GpuEmitter) == 64, "GpuEmitter debe ser de 64 bytes (igual que GPUEmitter en host)");
