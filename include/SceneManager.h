#pragma once
// =============================================================================
// SceneManager.h - Gestor de escenas
// Proyecto: Simulación de Fuego con Curl Noise
// =============================================================================

#include <vector>
#include <memory>
#include "Scene.h"
#include "FrameParticleScene.h"
#include "ParticleSystem.h"
#include "Camera.h"

class SceneManager {
public:
    SceneManager();
    ~SceneManager();

    // Inicializa todas las escenas
    void init();

    void setFrameParticleConfig(const FrameParticleSceneConfig& config);

    // Carga una escena específica
    bool loadScene(int index, ParticleSystem& particleSystem, Camera& camera, bool lightweight = false);

    // Obtener información de la escena actual
    int getCurrentSceneIndex() const { return currentSceneIndex; }
    std::string getCurrentSceneName() const;
    float getCurrentSceneDuration() const;
    glm::vec3 getBackgroundColor() const;
    float getCurrentEmitScale(float t) const;

    int getNumScenes() const { return static_cast<int>(scenes.size()); }

private:
    std::vector<std::unique_ptr<Scene>> scenes;
    int currentSceneIndex = -1;
    FrameParticleSceneConfig frameParticleConfig;
};
