#pragma once

#include "Scene.h"

#include <memory>
#include <string>

struct FrameParticleSceneConfig {
    bool enabled = false;
    std::string inputPath;
    bool motionFromFrames = false;
    int maxObjectParticles = 4000000;
    int sampleStrideOverride = 0;
    int superSample = 1;   // particulas por pixel = superSample^2 (mas densidad / menos pixelado)
    float worldWidth = 3.1f;
    float duration = 26.0f;
    bool hasIgnitionPoint = false;
    bool ignitionUsesPixels = false;
    float ignitionNormX = 0.5f;
    float ignitionNormY = 0.5f;
    float ignitionRadius = 0.032f;
};

std::unique_ptr<Scene> createFrameParticleScene(const FrameParticleSceneConfig& config);
