// =============================================================================
// Scenes.cpp - Implementa la fábrica incluyendo las 5 escenas existentes.
// Se incluyen los .cpp originales (cada uno define una clase Scene) para no
// duplicar la lógica de geometría/emisores. GeometryUtils se compila aparte
// (src/scenes/SceneUtils.cpp).
// =============================================================================
#include "Scenes.h"

#include <algorithm>
#include <cctype>
#include <cstdlib>   // rand() usado por algunas escenas
#include <cmath>     // cos/sin usados por algunas escenas

// Las escenas viven en src/scenes/ (single source of truth).
#include "scenes/PaperFireScene.cpp"
#include "scenes/WallFireScene.cpp"
#include "scenes/TreeFireScene.cpp"
#include "scenes/StructuralFireScene.cpp"
#include "scenes/BuildingFireScene.cpp"

static const char* kNames[] = {
    "PaperFire", "WallFire", "TreeFire", "StructuralFire", "BuildingFire"
};

std::unique_ptr<Scene> createScene(int index) {
    switch (index) {
        case 0: return std::make_unique<PaperFireScene>();
        case 1: return std::make_unique<WallFireScene>();
        case 2: return std::make_unique<TreeFireScene>();
        case 3: return std::make_unique<StructuralFireScene>();
        case 4: return std::make_unique<BuildingFireScene>();
        default: return nullptr;
    }
}

int sceneCount() { return 5; }

const char* sceneName(int index) {
    if (index < 0 || index >= 5) return "Unknown";
    return kNames[index];
}

int sceneIndexFromName(const std::string& name) {
    std::string n = name;
    std::transform(n.begin(), n.end(), n.begin(),
                   [](unsigned char c) { return (char)std::tolower(c); });

    if (n == "paperfire" || n == "paper" || n == "papel")      return 0;
    if (n == "wallfire"  || n == "wall"  || n == "pared")      return 1;
    if (n == "treefire"  || n == "tree"  || n == "arbol")      return 2;
    if (n == "structuralfire" || n == "structural" || n == "estructura") return 3;
    if (n == "buildingfire"   || n == "building"   || n == "edificio" || n == "utec") return 4;

    // ¿Número?
    try {
        size_t consumed = 0;
        int idx = std::stoi(name, &consumed);
        if (consumed == name.size() && idx >= 0 && idx < 5) return idx;
    } catch (...) {}

    return -1;
}
