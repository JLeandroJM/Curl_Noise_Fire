cmake_minimum_required(VERSION 3.20)
project(curl_noise_fire CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Release)
endif()

if(NOT APPLE)
    message(FATAL_ERROR "This Metal build only supports Apple platforms.")
endif()

find_library(METAL_LIB        Metal       REQUIRED)
find_library(METALKIT_LIB     MetalKit    REQUIRED)
find_library(FOUNDATION_LIB   Foundation  REQUIRED)
find_library(QUARTZCORE_LIB   QuartzCore  REQUIRED)
find_library(APPKIT_LIB       AppKit      REQUIRED)
find_library(COREGRAPHICS_LIB CoreGraphics REQUIRED)

add_executable(fire
    src/main.cpp
    src/MetalCppImpl.cpp
    src/MetalContext.cpp
    src/Renderer.cpp
    src/ParticleSystem.cpp
    src/Camera.cpp
)

target_include_directories(fire PRIVATE
    ${CMAKE_SOURCE_DIR}/src
    ${CMAKE_SOURCE_DIR}/external/metal-cpp
    /opt/homebrew/include
)

target_link_libraries(fire PRIVATE
    ${METAL_LIB}
    ${METALKIT_LIB}
    ${FOUNDATION_LIB}
    ${QUARTZCORE_LIB}
    ${APPKIT_LIB}
    ${COREGRAPHICS_LIB}
)

target_compile_options(fire PRIVATE
    -Wall -Wextra -Wno-unused-parameter -Wno-deprecated-declarations
)

# -------- Embed the path to the .metal source so we can compile it at
# -------- runtime via MTL::Device::newLibrary( source ).  This avoids the
# -------- Metal Toolchain (xcrun metal) dependency, which is not shipped
# -------- with the bare Command Line Tools on recent macOS.  When/if you
# -------- want AOT compilation, restore an `xcrun metal -c` step here.

target_compile_definitions(fire PRIVATE
    FIRE_SHADER_PATH=\"${CMAKE_SOURCE_DIR}/shaders/particle.metal\"
)
