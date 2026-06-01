// =============================================================================
// main.cpp - Driver headless del motor CUDA de fuego.
// Simula una escena a dt fijo y exporta una secuencia de PNG RGBA a disco.
// No abre ninguna ventana: pensado para correr en el cluster Khipu (Slurm).
// =============================================================================
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <string>
#include <vector>
#include <filesystem>

#include "Particle.h"
#include "Camera.h"
#include "Scenes.h"
#include "FireEngine.h"

#include "stb_image_write.h"  // sólo prototipos (la implementación está en stb_impl.cpp)

namespace fs = std::filesystem;

struct Options {
    int scene = 0;
    int width = 1920;
    int height = 1080;
    int fps = 30;
    float duration = -1.0f;       // <0 => usar la duración de la escena
    std::string outdir = "output";
    bool lightweight = false;
    bool showGeometry = false;
    float windStrength = 0.0f;
    RenderParams rp;
};

static void printUsage() {
    std::printf(
        "\nSimulacion de Fuego (CUDA, headless)\n"
        "Uso: fire_cuda --scene <nombre|idx> [opciones]\n\n"
        "Escenas: PaperFire(0) WallFire(1) TreeFire(2) StructuralFire(3) BuildingFire(4)\n\n"
        "Opciones:\n"
        "  --scene <s>        Escena por nombre o indice (def: 0)\n"
        "  --width <n>        Ancho en px (def: 1920)\n"
        "  --height <n>       Alto en px (def: 1080)\n"
        "  --fps <n>          Frames por segundo (def: 30)\n"
        "  --duration <s>     Duracion en segundos (def: la de la escena)\n"
        "  --outdir <ruta>    Carpeta de salida de los PNG (def: output)\n"
        "  --lightweight      Menos particulas (pruebas / GPU chica)\n"
        "  --show-geometry    Dibuja materiales estaticos (preview, NO para composicion)\n"
        "  --wind <f>         Fuerza de viento en +X (def: 0)\n"
        "  --glow <f>         Brillo emisivo (def: 1.2)\n"
        "  --exposure <f>     Exposicion del tonemap (def: 1.3)\n"
        "  --bloom <f>        Intensidad del bloom (def: 0.8)\n"
        "  --bloom-iters <n>  Pasadas de blur del bloom (def: 5)\n"
        "  --smoke <f>        Densidad/opacidad del humo (def: 1.6)\n"
        "  --simple-raster    Usa el splat simple en vez del rasterizador tiled (fallback)\n"
        "  --help\n\n");
}

static bool needArg(int i, int argc, const char* flag) {
    if (i + 1 >= argc) { std::fprintf(stderr, "Falta valor para %s\n", flag); return false; }
    return true;
}

int main(int argc, char** argv) {
    Options o;

    for (int i = 1; i < argc; ++i) {
        std::string a = argv[i];
        if (a == "--help" || a == "-h") { printUsage(); return 0; }
        else if (a == "--scene" && needArg(i, argc, "--scene")) {
            int idx = sceneIndexFromName(argv[++i]);
            if (idx < 0) { std::fprintf(stderr, "Escena desconocida: %s\n", argv[i]); return 1; }
            o.scene = idx;
        }
        else if (a == "--width"  && needArg(i, argc, "--width"))  o.width  = std::atoi(argv[++i]);
        else if (a == "--height" && needArg(i, argc, "--height")) o.height = std::atoi(argv[++i]);
        else if (a == "--fps"    && needArg(i, argc, "--fps"))    o.fps    = std::atoi(argv[++i]);
        else if (a == "--duration" && needArg(i, argc, "--duration")) o.duration = (float)std::atof(argv[++i]);
        else if (a == "--outdir" && needArg(i, argc, "--outdir")) o.outdir = argv[++i];
        else if (a == "--lightweight")   o.lightweight = true;
        else if (a == "--show-geometry") o.showGeometry = true;
        else if (a == "--wind"   && needArg(i, argc, "--wind"))   o.windStrength = (float)std::atof(argv[++i]);
        else if (a == "--glow"   && needArg(i, argc, "--glow"))   o.rp.particleGlow = (float)std::atof(argv[++i]);
        else if (a == "--exposure" && needArg(i, argc, "--exposure")) o.rp.exposure = (float)std::atof(argv[++i]);
        else if (a == "--bloom"  && needArg(i, argc, "--bloom"))  o.rp.bloomIntensity = (float)std::atof(argv[++i]);
        else if (a == "--bloom-iters" && needArg(i, argc, "--bloom-iters")) o.rp.bloomIterations = std::atoi(argv[++i]);
        else if (a == "--smoke"  && needArg(i, argc, "--smoke"))  o.rp.smokeDensity = (float)std::atof(argv[++i]);
        else if (a == "--simple-raster") o.rp.tiled = false;
        else { std::fprintf(stderr, "Argumento desconocido: %s\n", a.c_str()); printUsage(); return 1; }
    }
    o.rp.showGeometry = o.showGeometry;

    // --- Construir la escena en el host ---
    auto scene = createScene(o.scene);
    if (!scene) { std::fprintf(stderr, "No se pudo crear la escena %d\n", o.scene); return 1; }

    std::vector<Particle> particles;
    std::vector<EmitterConfig> emitters;
    CurlNoiseParams curl;
    CameraPath cameraPath;
    scene->setup(particles, emitters, curl, cameraPath, o.lightweight);

    float duration = o.duration > 0.0f ? o.duration : scene->getDuration();
    int totalFrames = (int)(duration * o.fps + 0.5f);
    float dt = 1.0f / (float)o.fps;

    std::printf("========================================\n");
    std::printf("Escena      : %s (%d)\n", sceneName(o.scene), o.scene);
    std::printf("Resolucion  : %dx%d @ %d fps\n", o.width, o.height, o.fps);
    std::printf("Duracion    : %.2f s  (%d frames)\n", duration, totalFrames);
    std::printf("Particulas  : %zu | Emisores: %zu\n", particles.size(), emitters.size());
    std::printf("Salida      : %s/%s_%%05d.png (RGBA premultiplicado)\n", o.outdir.c_str(), sceneName(o.scene));
    std::printf("========================================\n");

    // --- Cámara y motor ---
    Camera cam;
    cam.setPath(cameraPath);

    FireEngine engine;
    engine.init(o.width, o.height, particles, emitters, curl,
                glm::vec3(1.0f, 0.0f, 0.0f), o.windStrength);

    std::error_code ec;
    fs::create_directories(o.outdir, ec);

    float aspect = (float)o.width / (float)o.height;
    std::vector<uint8_t> rgba;

    for (int f = 0; f < totalFrames; ++f) {
        float t = f * dt;

        cam.update(t);
        glm::mat4 view = cam.getViewMatrix();
        glm::mat4 proj = cam.getProjectionMatrix(aspect);
        glm::mat4 vp = proj * view;
        float proj11 = proj[1][1];

        engine.update(dt, t);
        engine.render(vp, proj11, o.rp, rgba);

        char path[1024];
        std::snprintf(path, sizeof(path), "%s/%s_%05d.png",
                      o.outdir.c_str(), sceneName(o.scene), f);
        if (!stbi_write_png(path, o.width, o.height, 4, rgba.data(), o.width * 4)) {
            std::fprintf(stderr, "Error escribiendo %s\n", path);
            return 1;
        }

        if (f % o.fps == 0 || f == totalFrames - 1) {
            std::printf("\rframe %d/%d (t=%.2fs)        ", f + 1, totalFrames, t);
            std::fflush(stdout);
        }
    }

    std::printf("\nListo. %d frames en %s/\n", totalFrames, o.outdir.c_str());
    engine.shutdown();
    return 0;
}
