#include "ParticleSystem.hpp"

#include "MetalContext.hpp"

#include <cstdio>
#include <cstring>
#include <random>

ParticleSystem::ParticleSystem(MetalContext& ctx, uint32_t particleCount)
    : ctx_(ctx), count_(particleCount) {}

ParticleSystem::~ParticleSystem() {
    if (updatePipeline_) updatePipeline_->release();
    if (particleBuffer_) particleBuffer_->release();
}

bool ParticleSystem::init() {
    MTL::Device*  device = ctx_.device();
    MTL::Library* lib    = ctx_.library();
    if (!device || !lib) return false;

    NS::String* fnName =
        NS::String::string("update_particles", NS::UTF8StringEncoding);
    MTL::Function* fn = lib->newFunction(fnName);
    if (!fn) {
        std::fprintf(stderr, "Could not find kernel 'update_particles'\n");
        return false;
    }

    NS::Error* err = nullptr;
    updatePipeline_ = device->newComputePipelineState(fn, &err);
    fn->release();
    if (!updatePipeline_) {
        std::fprintf(stderr, "Compute pipeline build failed: %s\n",
                     err ? err->localizedDescription()->utf8String()
                         : "(no error info)");
        return false;
    }

    threadgroupSize_ = updatePipeline_->maxTotalThreadsPerThreadgroup();
    if (threadgroupSize_ > 256) threadgroupSize_ = 256;

    initParticleBuffer();
    return particleBuffer_ != nullptr;
}

void ParticleSystem::initParticleBuffer() {
    const size_t bytes = sizeof(Particle) * count_;
    particleBuffer_ = ctx_.device()->newBuffer(
        bytes, MTL::ResourceStorageModeShared);
    if (!particleBuffer_) {
        std::fprintf(stderr, "Particle buffer allocation failed (%zu B)\n",
                     bytes);
        return;
    }

    // Seed every particle with a random staggered age so the system reaches
    // steady-state immediately instead of all spawning at once. The actual
    // respawn logic lives in the kernel.
    Particle* p = reinterpret_cast<Particle*>(particleBuffer_->contents());

    std::mt19937 rng(0xC0FFEEu);
    std::uniform_real_distribution<float> u01(0.0f, 1.0f);

    for (uint32_t i = 0; i < count_; ++i) {
        const float life = lifetimeMin + u01(rng) * (lifetimeMax - lifetimeMin);
        p[i].position[0] = emitterPos[0];
        p[i].position[1] = emitterPos[1];
        p[i].position[2] = emitterPos[2];
        p[i].age         = u01(rng) * life;   // staggered start
        p[i].velocity[0] = 0.0f;
        p[i].velocity[1] = 0.0f;
        p[i].velocity[2] = 0.0f;
        p[i].lifetime    = life;
        p[i].size        = sizeMin + u01(rng) * (sizeMax - sizeMin);
        p[i].seed        = u01(rng) * 1.0e4f + static_cast<float>(i);
        p[i]._pad0       = 0.0f;
        p[i]._pad1       = 0.0f;
    }
}

void ParticleSystem::encodeUpdate(MTL::CommandBuffer* cmd, float dt,
                                  float time, uint32_t frameIndex) {
    SimUniforms u{};
    u.dt             = dt;
    u.time           = time;
    u.frameIndex     = frameIndex;
    u.particleCount  = count_;
    u.emitterPos[0]  = emitterPos[0];
    u.emitterPos[1]  = emitterPos[1];
    u.emitterPos[2]  = emitterPos[2];
    u.emitVelMin     = emitVelMin;
    u.emitVelMax     = emitVelMax;
    u.emitRadius     = emitRadius;
    u.buoyancy       = buoyancy;
    u.curlScale      = curlScale;
    u.curlStrength   = curlStrength;
    u.lifetimeMin    = lifetimeMin;
    u.lifetimeMax    = lifetimeMax;
    u.sizeMin        = sizeMin;
    u.sizeMax        = sizeMax;

    MTL::ComputeCommandEncoder* enc = cmd->computeCommandEncoder();
    enc->setComputePipelineState(updatePipeline_);
    enc->setBuffer(particleBuffer_, 0, 0);
    enc->setBytes(&u, sizeof(u), 1);

    const NS::UInteger total = count_;
    const NS::UInteger tg    = threadgroupSize_;
    const NS::UInteger groups = (total + tg - 1) / tg;

    enc->dispatchThreadgroups(MTL::Size(groups, 1, 1),
                              MTL::Size(tg, 1, 1));
    enc->endEncoding();
}
