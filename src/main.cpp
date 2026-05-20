// Pure-C++ entry point. AppKit / MetalKit access happens through metal-cpp's
// extension headers (no Objective-C source files in this project).

#include <AppKit/AppKit.hpp>
#include <Foundation/Foundation.hpp>
#include <Metal/Metal.hpp>
#include <MetalKit/MetalKit.hpp>
#include <QuartzCore/QuartzCore.hpp>

#include <cstdio>
#include <cstdlib>
#include <string>

#include "MetalContext.hpp"
#include "ParticleSystem.hpp"
#include "Renderer.hpp"

// ===========================================================================
//                           PROJECT-WIDE PARAMETERS
// Tweak these and recompile. (No ImGui at this stage by design.)
// ===========================================================================
namespace cfg {
constexpr uint32_t kParticleCount = 100'000;
constexpr int      kWindowWidth   = 1280;
constexpr int      kWindowHeight  = 720;
constexpr int      kTargetFPS     = 60;
}

// ---------------------------------------------------------------------------
//                           NSApplication delegate
// Owns the window, the MTKView, and the renderer. metal-cpp routes the
// NSApplicationDelegate selectors through NS::ApplicationDelegate's virtuals.
// ---------------------------------------------------------------------------
class FireAppDelegate : public NS::ApplicationDelegate {
public:
    ~FireAppDelegate() override {
        delete renderer_;
        delete particles_;
        delete metal_;
        if (view_)   view_->release();
        if (window_) window_->release();
    }

    void applicationWillFinishLaunching(NS::Notification*) override {
        // Build the application menu so cmd-Q etc. work properly. (Without
        // this, the launched app has no menu bar and feels broken.)
        NS::Application* app = reinterpret_cast<NS::Application*>(
            NS::Application::sharedApplication());

        NS::Menu*     mainMenu = NS::Menu::alloc()->init();
        NS::MenuItem* appItem  = NS::MenuItem::alloc()->init();

        NS::Menu* appMenu = NS::Menu::alloc()->init(
            NS::String::string("Fire", NS::UTF8StringEncoding));

        NS::String* quitTitle =
            NS::String::string("Quit Fire", NS::UTF8StringEncoding);
        NS::String* quitKey =
            NS::String::string("q", NS::UTF8StringEncoding);
        SEL quitSel = sel_registerName("terminate:");

        NS::MenuItem* quitItem =
            appMenu->addItem(quitTitle, quitSel, quitKey);
        quitItem->setKeyEquivalentModifierMask(NS::EventModifierFlagCommand);

        appItem->setSubmenu(appMenu);
        mainMenu->addItem(appItem);
        app->setMainMenu(mainMenu);

        appMenu->release();
        appItem->release();
        mainMenu->release();
    }

    void applicationDidFinishLaunching(NS::Notification*) override {
        // ---------------- Metal context + shader library ----------------
        metal_ = new MetalContext();
        if (!metal_->device()) {
            std::fprintf(stderr, "No Metal device. Exiting.\n");
            std::exit(1);
        }
        if (!metal_->loadShaderSource(FIRE_SHADER_PATH)) {
            std::exit(1);
        }
        std::fprintf(stderr, "Metal device: %s\n",
                     metal_->device()->name()->utf8String());

        // ---------------- Particle system + renderer ----------------
        particles_ = new ParticleSystem(*metal_, cfg::kParticleCount);
        if (!particles_->init()) {
            std::fprintf(stderr, "Particle system init failed.\n");
            std::exit(1);
        }

        renderer_ = new Renderer(*metal_, *particles_);

        // ---------------- Window + MTKView ----------------
        CGRect frame = (CGRect){ {100, 100},
                                 {(CGFloat)cfg::kWindowWidth,
                                  (CGFloat)cfg::kWindowHeight} };

        window_ = NS::Window::alloc()->init(
            frame,
            NS::WindowStyleMaskTitled
                | NS::WindowStyleMaskClosable
                | NS::WindowStyleMaskResizable
                | NS::WindowStyleMaskMiniaturizable,
            NS::BackingStoreBuffered,
            false);
        window_->setTitle(NS::String::string("Curl-Noise Fire",
                                             NS::UTF8StringEncoding));

        view_ = MTK::View::alloc()->init(frame, metal_->device());
        view_->setColorPixelFormat(MTL::PixelFormatBGRA8Unorm);
        view_->setClearColor(MTL::ClearColor::Make(0.0, 0.0, 0.0, 1.0));
        view_->setPreferredFramesPerSecond(cfg::kTargetFPS);

        if (!renderer_->init(view_->colorPixelFormat())) {
            std::fprintf(stderr, "Renderer init failed.\n");
            std::exit(1);
        }

        const float aspect = float(cfg::kWindowWidth) / float(cfg::kWindowHeight);
        renderer_->setAspect(aspect);
        view_->setDelegate(renderer_);

        window_->setContentView(reinterpret_cast<NS::View*>(view_));
        window_->makeKeyAndOrderFront(nullptr);

        NS::Application* app = reinterpret_cast<NS::Application*>(
            NS::Application::sharedApplication());
        app->activateIgnoringOtherApps(true);
    }

    bool applicationShouldTerminateAfterLastWindowClosed(NS::Application*) override {
        return true;
    }

private:
    MetalContext*   metal_     = nullptr;
    ParticleSystem* particles_ = nullptr;
    Renderer*       renderer_  = nullptr;
    NS::Window*     window_    = nullptr;
    MTK::View*      view_      = nullptr;
};

// ---------------------------------------------------------------------------

int main(int, const char* []) {
    NS::AutoreleasePool* pool = NS::AutoreleasePool::alloc()->init();

    FireAppDelegate delegate;

    NS::Application* app = NS::Application::sharedApplication();
    app->setDelegate(&delegate);
    app->setActivationPolicy(NS::ActivationPolicyRegular);
    app->run();

    pool->release();
    return 0;
}
