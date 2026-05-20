#pragma once

#include <cstdint>

// Layout shared with shaders/particle.metal. The MSL side uses packed_float3
// so each vec3 is 12 bytes and the next float fills the 4 trailing bytes of
// a 16-byte slot. Total = 48 bytes, naturally 16-byte aligned. If you change
// either side you MUST change the other.
//
// Field order on host MUST mirror the MSL Particle struct exactly.

struct alignas(16) Particle {
    float position[3];   // 12B  -> offset 0
    float age;           //  4B  -> offset 12      (current age in seconds)
    float velocity[3];   // 12B  -> offset 16
    float lifetime;      //  4B  -> offset 28      (total lifetime in seconds)
    float size;          //  4B  -> offset 32      (billboard half-size, world units)
    float seed;          //  4B  -> offset 36      (per-particle PRNG seed)
    float _pad0;         //  4B  -> offset 40
    float _pad1;         //  4B  -> offset 44
};
static_assert(sizeof(Particle) == 48, "Particle layout must match MSL side");
static_assert(alignof(Particle) == 16, "Particle alignment must be 16");

// Uniforms consumed by the compute kernel (one per dispatch).
struct alignas(16) SimUniforms {
    float dt;
    float time;
    uint32_t frameIndex;
    uint32_t particleCount;

    float emitterPos[3];
    float _pad0;

    float emitVelMin;
    float emitVelMax;
    float emitRadius;
    float buoyancy;

    float curlScale;
    float curlStrength;
    float lifetimeMin;
    float lifetimeMax;

    float sizeMin;
    float sizeMax;
    float _pad1;
    float _pad2;
};
static_assert(sizeof(SimUniforms) == 80, "SimUniforms layout must match MSL side");

// Uniforms consumed by the billboard vertex shader.
struct alignas(16) CameraUniforms {
    float viewProj[16];     // column-major mat4
    float cameraRight[3];
    float _pad0;
    float cameraUp[3];
    float _pad1;
};
static_assert(sizeof(CameraUniforms) == 96, "CameraUniforms layout must match MSL side");
