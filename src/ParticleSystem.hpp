#pragma once

#include <Metal/Metal.hpp>

#include <cstdint>

#include "Particle.hpp"

class MetalContext;

// Owns the GPU buffer of particles, the compute pipeline that integrates
// them, and the simulation uniforms. The rendering side reads the particle
// buffer through bufferForRendering() to avoid duplicating storage.
class ParticleSystem {
public:
    ParticleSystem(MetalContext& ctx, uint32_t particleCount);
    ~ParticleSystem();

    bool init();   // builds pipeline + initializes the buffer

    // Encode the compute dispatch that advances the simulation by `dt`.
    void encodeUpdate(MTL::CommandBuffer* cmd, float dt, float time,
                      uint32_t frameIndex);

    MTL::Buffer* bufferForRendering() const { return particleBuffer_; }
    uint32_t     particleCount()     const { return count_; }

    // ----- Tunable simulation parameters (mirrored to SimUniforms) -----
    float emitterPos[3] = {0.0f, 0.0f, 0.0f};
    float emitVelMin   = 0.5f;
    float emitVelMax   = 1.5f;
    float emitRadius   = 0.10f;
    float buoyancy     = 2.0f;
    float curlScale    = 1.0f;
    float curlStrength = 1.5f;
    float lifetimeMin  = 1.0f;
    float lifetimeMax  = 3.0f;
    float sizeMin      = 0.04f;
    float sizeMax      = 0.10f;

private:
    void initParticleBuffer();

    MetalContext&             ctx_;
    uint32_t                  count_;
    MTL::Buffer*              particleBuffer_  = nullptr;
    MTL::ComputePipelineState* updatePipeline_ = nullptr;
    NS::UInteger              threadgroupSize_ = 64;
};
