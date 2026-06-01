cmake_minimum_required(VERSION 3.18)

project(FireCUDA VERSION 1.0 LANGUAGES CXX CUDA)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CUDA_STANDARD 17)
set(CMAKE_CUDA_STANDARD_REQUIRED ON)

# Arquitecturas GPU:
#   GTX 1650 / Tesla T4 -> 75
#   A100                -> 80
#   RTX A6000           -> 86
# Sobreescribir con, p.ej., -DCMAKE_CUDA_ARCHITECTURES=75 al configurar.
if(NOT DEFINED CMAKE_CUDA_ARCHITECTURES)
    set(CMAKE_CUDA_ARCHITECTURES 75 80 86)
endif()

# ------------------------------------------------------------
# Dependencias locales (todo vendorizado en external/, sin internet)
# ------------------------------------------------------------
set(GLM_DIR "${CMAKE_CURRENT_SOURCE_DIR}/external/glm")
set(STB_DIR "${CMAKE_CURRENT_SOURCE_DIR}/external/stb")

if(NOT EXISTS "${GLM_DIR}/glm/glm.hpp")
    message(FATAL_ERROR "No se encontro ${GLM_DIR}/glm/glm.hpp")
endif()
if(NOT EXISTS "${STB_DIR}/stb_image_write.h")
    message(FATAL_ERROR "No se encontro ${STB_DIR}/stb_image_write.h")
endif()

# ------------------------------------------------------------
# Fuentes
# ------------------------------------------------------------
add_executable(fire_cuda
    src/main.cpp
    src/Scenes.cpp
    src/stb_impl.cpp
    src/FireEngine.cu
    src/Camera.cpp
    src/scenes/SceneUtils.cpp
)

target_include_directories(fire_cuda PRIVATE
    "${CMAKE_CURRENT_SOURCE_DIR}/include"
    "${CMAKE_CURRENT_SOURCE_DIR}/src"
    "${GLM_DIR}"
    "${STB_DIR}"
)

# Camera.cpp usa glm/gtx (lerp) -> experimental
target_compile_definitions(fire_cuda PRIVATE GLM_ENABLE_EXPERIMENTAL)

# Optimizacion
target_compile_options(fire_cuda PRIVATE
    $<$<COMPILE_LANGUAGE:CUDA>:--use_fast_math>
)

set_target_properties(fire_cuda PROPERTIES
    CUDA_SEPARABLE_COMPILATION OFF
)

message(STATUS "FireCUDA: arquitecturas CUDA = ${CMAKE_CUDA_ARCHITECTURES}")
message(STATUS "FireCUDA: glm desde ${GLM_DIR}")
