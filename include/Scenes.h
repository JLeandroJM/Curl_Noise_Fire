// =============================================================================
// Scenes.h - Fábrica de las 5 escenas del proyecto (reutiliza src/scenes/*).
// =============================================================================
#pragma once

#include <memory>
#include <string>
#include "Scene.h"

// Crea la escena por índice (0..4): Paper, Wall, Tree, Structural, Building.
std::unique_ptr<Scene> createScene(int index);

int          sceneCount();
const char*  sceneName(int index);
int          sceneIndexFromName(const std::string& name); // -1 si no existe
