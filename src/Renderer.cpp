#include "Renderer.hpp"

#include "MetalContext.hpp"
#include "ParticleSystem.hpp"
#include "Particle.hpp"

#include <glm/gtc/type_ptr.hpp>

#include <cstdio>
#include <cstring>

Renderer::Renderer(MetalContext& ctx, ParticleSystem& particles)
    : ctx_(ctx), particles_(particles) {
    startTime_ = Clock::now();
    lastFrame_ = startTime_;
    camera_ = std::make_unique<Camera>(
        glm::vec3(0.0f, 2.0f, 5.0f),
        glm::vec3(0.0f, 1.0f, 0.0f),     // look slightly above the emitter
        glm::vec3(0.0f, 1.0f, 0.0f),
        glm::radians(60.0f),
        16.0f / 9.0f,
        0.1f, 100.0f);
}

Renderer::~Renderer() {
    if (renderPipeline_) renderPipeline_->release();
    if (depthState_)     depthState_->release();
}

bool Renderer::init(MTL::PixelFormat colorFormat) {
    MTL::Device*  device = ctx_.device();
    MTL::Library* lib    = ctx_.library();
    if (!device || !lib) return false;

    NS::String* vName =
        NS::String::string("billboard_vertex",   NS::UTF8StringEncoding);
    NS::String* fName =
        NS::String::string("billboard_fragment", NS::UTF8StringEncoding);

    MTL::Function* vfn = lib->newFunction(vName);
    MTL::Function* ffn = lib->newFunction(fName);
    if (!vfn || !ffn) {
        std::fprintf(stderr, "Missing billboard vertex/fragment function\n");
        if (vfn) vfn->release();
        if (ffn) ffn->release();
        return false;
    }

    MTL::RenderPipelineDescriptor* desc =
        MTL::RenderPipelineDescriptor::alloc()->init();
    desc->setVertexFunction(vfn);
    desc->setFragmentFunction(ffn);

    MTL::RenderPipelineColorAttachmentDescriptor* color =
        desc->colorAttachments()->object(0);
    color->setPixelFormat(colorFormat);
    color->setBlendingEnabled(true);
    // Additive blending: out = src.rgb * src.a + dst.rgb * 1.
    color->setRgbBlendOperation(MTL::BlendOperationAdd);
    color->setAlphaBlendOperation(MTL::BlendOperationAdd);
    color->setSourceRGBBlendFactor(MTL::BlendFactorSourceAlpha);
    color->setSourceAlphaBlendFactor(MTL::BlendFactorOne);
    color->setDestinationRGBBlendFactor(MTL::BlendFactorOne);
    color->setDestinationAlphaBlendFactor(MTL::BlendFactorOne);

    NS::Error* err = nullptr;
    renderPipeline_ = device->newRenderPipelineState(desc, &err);
    desc->release();
    vfn->release();
    ffn->release();
    if (!renderPipeline_) {
        std::fprintf(stderr, "Render pipeline build failed: %s\n",
                     err ? err->localizedDescription()->utf8String()
                         : "(no error info)");
        return false;
    }

    // No depth testing/writing: additive fire is order-independent and we
    // don't have any opaque geometry yet.
    MTL::DepthStencilDescriptor* dsDesc =
        MTL::DepthStencilDescriptor::alloc()->init();
    dsDesc->setDepthCompareFunction(MTL::CompareFunctionAlways);
    dsDesc->setDepthWriteEnabled(false);
    depthState_ = device->newDepthStencilState(dsDesc);
    dsDesc->release();

    return true;
}

void Renderer::setAspect(float aspect) {
    if (camera_) camera_->setAspect(aspect);
}

void Renderer::drawableSizeWillChange(MTK::View*, CGSize size) {
    if (size.height > 0) {
        setAspect(static_cast<float>(size.width) /
                  static_cast<float>(size.height));
    }
}

void Renderer::drawInMTKView(MTK::View* pView) {
    NS::AutoreleasePool* pool = NS::AutoreleasePool::alloc()->init();

    const auto now = Clock::now();
    float dt = std::chrono::duration<float>(now - lastFrame_).count();
    if (dt > 0.05f) dt = 0.05f;     // clamp huge dt (paused window etc.)
    const float t  = std::chrono::duration<float>(now - startTime_).count();
    lastFrame_ = now;

    MTL::CommandBuffer* cmd = ctx_.queue()->commandBuffer();

    // 1) Compute pass: advance the particles.
    particles_.encodeUpdate(cmd, dt, t, frameIndex_);

    // 2) Render pass: billboards.
    MTL::RenderPassDescriptor* rpd = pView->currentRenderPassDescriptor();
    if (rpd) {
        MTL::RenderCommandEncoder* enc = cmd->renderCommandEncoder(rpd);
        enc->setRenderPipelineState(renderPipeline_);
        enc->setDepthStencilState(depthState_);

        CameraUniforms cu{};
        const glm::mat4 vp = camera_->viewProj();
        std::memcpy(cu.viewProj, glm::value_ptr(vp), sizeof(cu.viewProj));
        const glm::vec3 r = camera_->cameraRight();
        const glm::vec3 u = camera_->cameraUp();
        cu.cameraRight[0] = r.x; cu.cameraRight[1] = r.y; cu.cameraRight[2] = r.z;
        cu.cameraUp[0]    = u.x; cu.cameraUp[1]    = u.y; cu.cameraUp[2]    = u.z;

        enc->setVertexBuffer(particles_.bufferForRendering(), 0, 0);
        enc->setVertexBytes(&cu, sizeof(cu), 1);

        // 6 vertices per particle (two triangles), N instances.
        enc->drawPrimitives(MTL::PrimitiveTypeTriangle,
                            NS::UInteger(0),
                            NS::UInteger(6),
                            NS::UInteger(particles_.particleCount()));
        enc->endEncoding();

        cmd->presentDrawable(pView->currentDrawable());
    }

    cmd->commit();
    ++frameIndex_;

    pool->release();
}
