#pragma once

#include <Foundation/Foundation.hpp>
#include <Metal/Metal.hpp>

#include <string>

// Thin wrapper around the Metal device, queue and the precompiled shader
// library. Everything Metal-specific that the rest of the engine touches
// goes through this object, so that when we port the GPU side to CUDA on
// Windows we only need to replace this layer plus ParticleSystem's command
// encoding methods.
class MetalContext {
public:
    MetalContext();
    ~MetalContext();

    MetalContext(const MetalContext&)            = delete;
    MetalContext& operator=(const MetalContext&) = delete;

    // Read the .metal source from `path` and compile it via the runtime
    // shader compiler. Returns false (and prints details) on failure.
    bool loadShaderSource(const std::string& path);

    MTL::Device*       device()  const { return device_; }
    MTL::CommandQueue* queue()   const { return queue_; }
    MTL::Library*      library() const { return library_; }

private:
    MTL::Device*       device_  = nullptr;
    MTL::CommandQueue* queue_   = nullptr;
    MTL::Library*      library_ = nullptr;
};
