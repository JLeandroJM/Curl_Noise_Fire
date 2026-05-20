#pragma once

#include <MetalKit/MetalKit.hpp>
#include <Metal/Metal.hpp>

#include <chrono>
#include <memory>

#include "Camera.hpp"

class MetalContext;
class ParticleSystem;

// Renderer is both the owner of the render pipeline and the MTKView delegate.
// MTKView drives drawInMTKView() at the display's refresh rate; we use that
// callback as our frame tick.
class Renderer : public MTK::ViewDelegate {
public:
    Renderer(MetalContext& ctx, ParticleSystem& particles);
    ~Renderer() override;

    bool init(MTL::PixelFormat colorFormat);
    void setAspect(float aspect);

    // MTKViewDelegate hooks (metal-cpp bridges these to the ObjC selectors).
    void drawInMTKView(MTK::View* pView) override;
    void drawableSizeWillChange(MTK::View* pView, CGSize size) override;

private:
    MetalContext&       ctx_;
    ParticleSystem&     particles_;

    MTL::RenderPipelineState* renderPipeline_ = nullptr;
    MTL::DepthStencilState*   depthState_     = nullptr;

    std::unique_ptr<Camera> camera_;

    using Clock = std::chrono::steady_clock;
    Clock::time_point startTime_;
    Clock::time_point lastFrame_;
    uint32_t          frameIndex_ = 0;
};
