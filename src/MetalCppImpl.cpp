// Single translation unit that emits the metal-cpp / AppKit / MetalKit
// implementation symbols. Defining these macros in exactly one .cpp file is
// the canonical way to use metal-cpp without an Objective-C runtime
// dependency on the host side.

#define NS_PRIVATE_IMPLEMENTATION
#define CA_PRIVATE_IMPLEMENTATION
#define MTL_PRIVATE_IMPLEMENTATION
#define MTK_PRIVATE_IMPLEMENTATION

#include <Foundation/Foundation.hpp>
#include <Metal/Metal.hpp>
#include <QuartzCore/QuartzCore.hpp>
#include <AppKit/AppKit.hpp>
#include <MetalKit/MetalKit.hpp>
