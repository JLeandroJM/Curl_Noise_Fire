#include "MetalContext.hpp"

#include <cstdio>
#include <fstream>
#include <sstream>

MetalContext::MetalContext() {
    device_ = MTL::CreateSystemDefaultDevice();
    if (!device_) {
        std::fprintf(stderr, "Metal: no default device available\n");
        return;
    }
    queue_ = device_->newCommandQueue();
}

MetalContext::~MetalContext() {
    if (library_) library_->release();
    if (queue_)   queue_->release();
    if (device_)  device_->release();
}

bool MetalContext::loadShaderSource(const std::string& path) {
    if (!device_) return false;

    std::ifstream f(path);
    if (!f.is_open()) {
        std::fprintf(stderr, "Could not open shader source: %s\n", path.c_str());
        return false;
    }
    std::ostringstream ss;
    ss << f.rdbuf();
    const std::string src = ss.str();

    NS::String* nsSrc = NS::String::string(src.c_str(), NS::UTF8StringEncoding);

    MTL::CompileOptions* opts = MTL::CompileOptions::alloc()->init();
    opts->setFastMathEnabled(true);

    NS::Error* err = nullptr;
    library_ = device_->newLibrary(nsSrc, opts, &err);
    opts->release();

    if (!library_) {
        std::fprintf(stderr,
                     "Shader compile failed (%s):\n%s\n",
                     path.c_str(),
                     err ? err->localizedDescription()->utf8String()
                         : "(no error info)");
        return false;
    }
    return true;
}
