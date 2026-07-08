#ifndef GLFW_INCLUDE_NONE
#define GLFW_INCLUDE_NONE
#endif

#include <glad/gl.h>
#include <GLFW/glfw3.h>

#include <glm/glm.hpp>

#include <algorithm>
#include <cctype>
#include <filesystem>
#include <iomanip>
#include <iostream>
#include <sstream>
#include <stdexcept>
#include <string>

#include "Renderer.h"
#include "SceneManager.h"
#include "ParticleSystem.h"
#include "Camera.h"
#include "FrameExporter.h"

std::string toLower(std::string s) {
    std::transform(
        s.begin(),
        s.end(),
        s.begin(),
        [](unsigned char c) {
            return static_cast<char>(std::tolower(c));
        }
    );

    return s;
}

int parseSceneArgument(const std::string& value) {
    try {
        size_t parsedChars = 0;
        int sceneIndex = std::stoi(value, &parsedChars);

        if (parsedChars == value.size()) {
            return sceneIndex;
        }
    } catch (...) {
        // Si no es número, probamos por nombre.
    }

    std::string name = toLower(value);

    if (name == "paperfire" || name == "paper" || name == "papel") {
        return 0;
    }

    if (name == "wallfire" || name == "wall" || name == "pared") {
        return 1;
    }

    if (name == "treefire" || name == "tree" || name == "arbol" || name == "árbol") {
        return 2;
    }

    if (name == "structuralfire" || name == "structural" || name == "estructura") {
        return 3;
    }

    if (name == "buildingfire" || name == "building" || name == "edificio" || name == "utec") {
        return 4;
    }

    if (name == "test5" || name == "scene5" || name == "papelpro" || name == "papelrojo") {
        return 5;
    }

    std::cerr << "Escena no reconocida: " << value << std::endl;
    std::cerr << "Usando escena 0 por defecto.\n";

    return 0;
}

void printUsage() {
    std::cout << "\nUso:\n";
    std::cout << "  FireSimulation.exe --scene 0 --preview --lightweight\n";
    std::cout << "  FireSimulation.exe --scene PaperFire --preview --lightweight\n";
    std::cout << "  FireSimulation.exe --scene BuildingFire --preview --lightweight\n\n";

    std::cout << "Escenas:\n";
    std::cout << "  0 / PaperFire      = fuego sobre papel / fogata simple\n";
    std::cout << "  1 / WallFire       = fuego sobre pared\n";
    std::cout << "  2 / TreeFire       = fuego en arbol\n";
    std::cout << "  3 / StructuralFire = fuego estructural\n";
    std::cout << "  4 / BuildingFire   = fuego en edificio\n";
    std::cout << "  5 / Test5          = papel quemándose / escena final de papel\n\n";

    std::cout << "Opciones:\n";
    std::cout << "  --preview       Ventana 1280x720\n";
    std::cout << "  --lightweight   Menos carga para GPU modesta\n";
    std::cout << "  --export        Exporta video/frames segun soporte\n";
    std::cout << "                 En servidor guarda PNGs en ./frames aunque no haya OpenCV\n";
    std::cout << "  --help          Muestra esta ayuda\n\n";
}

void glfwErrorCallback(int error, const char* description) {
    std::cerr << "GLFW Error [" << error << "]: " << description << std::endl;
}

void framebuffer_size_callback(GLFWwindow* window, int width, int height) {
    if (width <= 0 || height <= 0) {
        return;
    }

    glViewport(0, 0, width, height);

    auto renderer = reinterpret_cast<Renderer*>(glfwGetWindowUserPointer(window));

    if (renderer) {
        renderer->resize(width, height);
    }
}

void processInput(GLFWwindow* window) {
    if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS) {
        glfwSetWindowShouldClose(window, true);
    }
}

void printOpenGLInfo() {
    const GLubyte* version = glGetString(GL_VERSION);
    const GLubyte* renderer = glGetString(GL_RENDERER);
    const GLubyte* vendor = glGetString(GL_VENDOR);

    std::cout << "\n========== OPENGL INFO ==========\n";
    std::cout << "OpenGL Version: "
              << (version ? reinterpret_cast<const char*>(version) : "NULL")
              << "\n";

    std::cout << "Renderer: "
              << (renderer ? reinterpret_cast<const char*>(renderer) : "NULL")
              << "\n";

    std::cout << "Vendor: "
              << (vendor ? reinterpret_cast<const char*>(vendor) : "NULL")
              << "\n";

    GLint maxComputeWorkGroupCountX = 0;
    GLint maxComputeWorkGroupSizeX = 0;

    glGetIntegeri_v(GL_MAX_COMPUTE_WORK_GROUP_COUNT, 0, &maxComputeWorkGroupCountX);
    glGetIntegeri_v(GL_MAX_COMPUTE_WORK_GROUP_SIZE, 0, &maxComputeWorkGroupSizeX);

    std::cout << "Max compute work groups X: " << maxComputeWorkGroupCountX << "\n";
    std::cout << "Max compute work group size X: " << maxComputeWorkGroupSizeX << "\n";
    std::cout << "=================================\n\n";
}

void drawDebugRedRectangle(int windowWidth, int windowHeight) {
    // Este bloque dibuja un rectángulo rojo directamente sobre el framebuffer principal.
    // Si esto aparece, la ventana y glfwSwapBuffers funcionan.
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    glViewport(0, 0, windowWidth, windowHeight);

    glDisable(GL_DEPTH_TEST);
    glDisable(GL_BLEND);

    glEnable(GL_SCISSOR_TEST);

    // Coordenadas desde abajo-izquierda.
    glScissor(30, 30, 260, 130);

    glClearColor(1.0f, 0.0f, 0.0f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT);

    glDisable(GL_SCISSOR_TEST);
}

void checkOpenGLError(const std::string& where) {
    GLenum err = glGetError();

    if (err != GL_NO_ERROR) {
        std::cerr << "OpenGL error en " << where << ": 0x"
                  << std::hex << err << std::dec << std::endl;
    }
}

int main(int argc, char** argv) {
    int initialScene = 0;
    bool lightweight = false;
    bool exportVideo = false;
    bool previewMode = false;

    int windowWidth = 1920;
    int windowHeight = 1080;

    // Cambia esto a false cuando ya confirmemos que el render funciona.
    bool debugRedRectangle = false;

    for (int i = 1; i < argc; ++i) {
        std::string arg = argv[i];

        if (arg == "--help" || arg == "-h") {
            printUsage();
            return 0;
        }

        if (arg == "--scene" && i + 1 < argc) {
            initialScene = parseSceneArgument(argv[++i]);
        } else if (arg == "--lightweight") {
            lightweight = true;
        } else if (arg == "--export") {
            exportVideo = true;
        } else if (arg == "--preview") {
            previewMode = true;
            windowWidth = 1280;
            windowHeight = 720;
        } else if (arg == "--no-debug-red") {
            debugRedRectangle = false;
        } else {
            std::cerr << "Argumento desconocido: " << arg << std::endl;
        }
    }

    std::cout << "========== CONFIG ==========\n";
    std::cout << "Scene index: " << initialScene << "\n";
    std::cout << "Preview: " << (previewMode ? "true" : "false") << "\n";
    std::cout << "Lightweight: " << (lightweight ? "true" : "false") << "\n";
    std::cout << "Export: " << (exportVideo ? "true" : "false") << "\n";
    std::cout << "Debug red rectangle: " << (debugRedRectangle ? "true" : "false") << "\n";
    std::cout << "Resolution: " << windowWidth << "x" << windowHeight << "\n";
    std::cout << "============================\n\n";

    glfwSetErrorCallback(glfwErrorCallback);

    if (!glfwInit()) {
        std::cerr << "Fallo al inicializar GLFW.\n";
        return -1;
    }

    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

    // En modo exportación no necesitamos mostrar la ventana.
    // Esto ayuda cuando se ejecuta en servidor con xvfb-run.
    if (exportVideo) {
        glfwWindowHint(GLFW_VISIBLE, GLFW_FALSE);
    }

    GLFWwindow* window = glfwCreateWindow(
        windowWidth,
        windowHeight,
        "Simulacion de Fuego - Curl Noise",
        nullptr,
        nullptr
    );

    if (!window) {
        std::cerr << "Fallo al crear la ventana GLFW.\n";
        glfwTerminate();
        return -1;
    }

    glfwMakeContextCurrent(window);

    // En exportación usamos dt fijo, no conviene sincronizar a monitor.
    glfwSwapInterval(exportVideo ? 0 : 1);

    if (!gladLoadGL((GLADloadfunc)glfwGetProcAddress)) {
        std::cerr << "Fallo al inicializar GLAD.\n";
        glfwDestroyWindow(window);
        glfwTerminate();
        return -1;
    }

    printOpenGLInfo();

    glViewport(0, 0, windowWidth, windowHeight);

    // Test inicial: limpia a gris oscuro antes de inicializar renderer.
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    glClearColor(0.08f, 0.08f, 0.10f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    glfwSwapBuffers(window);
    glfwPollEvents();

    Renderer renderer;

    std::cout << "Inicializando Renderer...\n";
    renderer.init(windowWidth, windowHeight, "shaders");
    checkOpenGLError("renderer.init");

    glfwSetWindowUserPointer(window, &renderer);
    glfwSetFramebufferSizeCallback(window, framebuffer_size_callback);

    SceneManager sceneManager;
    sceneManager.init();

    ParticleSystem particleSystem;
    Camera camera;

    std::cout << "Cargando escena " << initialScene << "...\n";

    if (!sceneManager.loadScene(initialScene, particleSystem, camera, lightweight)) {
        std::cerr << "No se pudo cargar la escena inicial " << initialScene << ".\n";
        std::cerr << "Intentando cargar escena 0...\n";

        if (!sceneManager.loadScene(0, particleSystem, camera, lightweight)) {
            std::cerr << "Tampoco se pudo cargar la escena 0. Abortando.\n";
            glfwDestroyWindow(window);
            glfwTerminate();
            return -1;
        }

        initialScene = 0;
    }

    float sceneDuration = sceneManager.getCurrentSceneDuration();
    glm::vec3 initialBg = sceneManager.getBackgroundColor();

    std::cout << "Escena cargada correctamente.\n";
    std::cout << "Duracion escena: " << sceneDuration << " segundos\n";
    std::cout << "Background color: "
              << initialBg.r << ", "
              << initialBg.g << ", "
              << initialBg.b << "\n\n";

    FrameExporter exporter;
    std::filesystem::path framesDir = "frames";

    if (exportVideo) {
        std::filesystem::create_directories(framesDir);

        std::string outputName = "output_scene" + std::to_string(initialScene) + ".mp4";

        std::cout << "Inicializando exportador: " << outputName << "\n";
        std::cout << "Tambien se guardaran PNGs en: " << framesDir.string() << "\n";

        // Si OpenCV no está disponible, esto puede fallar, pero NO detiene la exportación.
        // Igual guardaremos frames PNG y luego los convertimos con ffmpeg.
        exporter.initVideoWriter(outputName, windowWidth, windowHeight, 30.0);
    }

    float lastFrame = static_cast<float>(glfwGetTime());
    float simulatedTime = 0.0f;
    float dtFixed = 1.0f / 30.0f;

    int frameCount = 0;
    float fpsTimer = 0.0f;

    std::cout << "Entrando al loop principal...\n";
    std::cout << "Presiona ESC para cerrar.\n\n";

    while (!glfwWindowShouldClose(window)) {
        processInput(window);

        float currentRealTime = static_cast<float>(glfwGetTime());
        float deltaTime = currentRealTime - lastFrame;
        lastFrame = currentRealTime;

        if (deltaTime > 0.1f) {
            deltaTime = 0.1f;
        }

        float dt = exportVideo ? dtFixed : deltaTime;

        camera.update(simulatedTime);
        particleSystem.update(dt, simulatedTime);
        checkOpenGLError("particleSystem.update");

        glm::vec3 bgColor = sceneManager.getBackgroundColor();

        renderer.renderFrame(particleSystem, camera, bgColor);
        checkOpenGLError("renderer.renderFrame");

        if (debugRedRectangle) {
            drawDebugRedRectangle(windowWidth, windowHeight);
            checkOpenGLError("drawDebugRedRectangle");
        }

        if (exportVideo) {
            // 1) Intentar agregar el frame al video directo si OpenCV está disponible.
            exporter.addFrame(windowWidth, windowHeight);

            // 2) Guardar siempre PNG, para servidores sin OpenCV o sin salida gráfica real.
            std::ostringstream frameName;
            frameName << "frame_"
                      << std::setw(4)
                      << std::setfill('0')
                      << frameCount
                      << ".png";

            std::filesystem::path framePath = framesDir / frameName.str();

            if (!exporter.saveFrameAsPNG(framePath.string(), windowWidth, windowHeight)) {
                std::cerr << "No se pudo guardar PNG: " << framePath.string() << "\n";
            }

            if (simulatedTime >= sceneDuration) {
                std::cout << "Tiempo de escena completado. Cerrando export.\n";
                break;
            }
        }

        glfwSwapBuffers(window);
        glfwPollEvents();

        simulatedTime += dt;

        frameCount++;
        fpsTimer += deltaTime;

        if (fpsTimer >= 1.0f) {
            float fps = static_cast<float>(frameCount) / fpsTimer;

            std::cout << "FPS: " << fps
                      << " | t: " << simulatedTime
                      << " | scene: " << initialScene
                      << std::endl;

            frameCount = 0;
            fpsTimer = 0.0f;
        }
    }

    if (exportVideo) {
        exporter.finish();
        std::cout << "Export finalizado. Frames PNG guardados en ./frames\n";
        std::cout << "Para convertir a MP4 usa:\n";
        std::cout << "ffmpeg -y -framerate 30 -i frames/frame_%04d.png -c:v libx264 -pix_fmt yuv420p papel_scene"
                  << initialScene << ".mp4\n";
    }

    std::cout << "Cerrando aplicacion.\n";

    glfwDestroyWindow(window);
    glfwTerminate();

    return 0;
}