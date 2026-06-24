cmake_minimum_required(VERSION 3.16)

project(FireSimulation VERSION 1.0 LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# ------------------------------------------------------------
# FetchContent para dependencias
# ------------------------------------------------------------
include(FetchContent)

# ------------------------------------------------------------
# GLFW
# ------------------------------------------------------------
FetchContent_Declare(
    glfw
    GIT_REPOSITORY https://github.com/glfw/glfw.git
    GIT_TAG 3.3.8
)

set(GLFW_BUILD_DOCS OFF CACHE BOOL "" FORCE)
set(GLFW_BUILD_TESTS OFF CACHE BOOL "" FORCE)
set(GLFW_BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)

FetchContent_MakeAvailable(glfw)

# ------------------------------------------------------------
# ------------------------------------------------------------
# GLM LOCAL
# GLM es header-only, no se descarga ni se linkea.
# Debe existir:
#   external/glm/glm/glm.hpp
# ------------------------------------------------------------
set(GLM_DIR "${CMAKE_CURRENT_SOURCE_DIR}/external/glm")

if(NOT EXISTS "${GLM_DIR}/glm/glm.hpp")
    message(FATAL_ERROR "No se encontro external/glm/glm/glm.hpp")
endif()
# ------------------------------------------------------------
# OpenCV opcional
# Si no está instalado, el proyecto igual compila.
# Solo se desactiva exportación directa a video.
# ------------------------------------------------------------
find_package(OpenCV QUIET)

if(OpenCV_FOUND)
    message(STATUS "OpenCV encontrado. Exportacion de video habilitada.")
else()
    message(WARNING "OpenCV no encontrado. Se desactiva la exportacion a video. Solo se exportaran frames PNG.")
endif()

# ------------------------------------------------------------
# GLAD LOCAL
# Este proyecto usa GLAD moderno:
#   external/glad/include/glad/gl.h
#   external/glad/src/gl.c
# ------------------------------------------------------------
set(GLAD_DIR "${CMAKE_CURRENT_SOURCE_DIR}/external/glad")

if(NOT EXISTS "${GLAD_DIR}/include/glad/gl.h")
    message(FATAL_ERROR "No se encontro external/glad/include/glad/gl.h")
endif()

if(NOT EXISTS "${GLAD_DIR}/src/gl.c")
    message(FATAL_ERROR "No se encontro external/glad/src/gl.c")
endif()

add_library(glad STATIC
    "${GLAD_DIR}/src/gl.c"
)

target_include_directories(glad PUBLIC
    "${GLAD_DIR}/include"
)

# ------------------------------------------------------------
# STB IMAGE WRITE para exportar PNGs
# ------------------------------------------------------------
set(STB_DIR "${CMAKE_CURRENT_BINARY_DIR}/stb")
file(MAKE_DIRECTORY "${STB_DIR}")

if(NOT EXISTS "${STB_DIR}/stb_image_write.h")
    file(DOWNLOAD
        "https://raw.githubusercontent.com/nothings/stb/master/stb_image_write.h"
        "${STB_DIR}/stb_image_write.h"
        SHOW_PROGRESS
    )
endif()

# ------------------------------------------------------------
# Fuentes del proyecto
# ------------------------------------------------------------
file(GLOB_RECURSE SOURCES CONFIGURE_DEPENDS
    "${CMAKE_CURRENT_SOURCE_DIR}/src/*.cpp"
)

add_executable(FireSimulation
    ${SOURCES}
        include/PaperFireScene.h)

# ------------------------------------------------------------
# Includes
# ------------------------------------------------------------
target_include_directories(FireSimulation PRIVATE
    "${CMAKE_CURRENT_SOURCE_DIR}/include"
    "${CMAKE_CURRENT_SOURCE_DIR}/src"
    "${GLAD_DIR}/include"
    "${STB_DIR}"
    "${GLM_DIR}"
)

if(OpenCV_FOUND)
    target_include_directories(FireSimulation PRIVATE
        ${OpenCV_INCLUDE_DIRS}
    )

    target_compile_definitions(FireSimulation PRIVATE
        HAS_OPENCV=1
    )
endif()

# ------------------------------------------------------------
# IMPORTANTE:
# Evita que GLFW incluya OpenGL por su cuenta.
# Nosotros cargamos OpenGL usando GLAD.
# ------------------------------------------------------------
target_compile_definitions(FireSimulation PRIVATE
    GLFW_INCLUDE_NONE
GLM_ENABLE_EXPERIMENTAL
)

# ------------------------------------------------------------
# Links
# OJO: NO poner glm aquí.
# GLM es header-only, por eso NO existe glm.lib.
# ------------------------------------------------------------
target_link_libraries(FireSimulation PRIVATE
    glad
    glfw
)

if(OpenCV_FOUND)
    target_link_libraries(FireSimulation PRIVATE
        ${OpenCV_LIBS}
    )
endif()

# ------------------------------------------------------------
# Copiar shaders al directorio del ejecutable
# ------------------------------------------------------------
add_custom_command(TARGET FireSimulation POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_directory
        "${CMAKE_CURRENT_SOURCE_DIR}/shaders"
        "$<TARGET_FILE_DIR:FireSimulation>/shaders"
)

# ------------------------------------------------------------
# Mensajes finales
# ------------------------------------------------------------
message(STATUS "GLAD local usado desde: ${GLAD_DIR}")
message(STATUS "GLM usado como headers desde: ${GLM_DIR}")
message(STATUS "Shaders se copiaran desde: ${CMAKE_CURRENT_SOURCE_DIR}/shaders")