#pragma once
// =============================================================================
// TreeScene.h - Árbol ardiendo: las hojas se incendian (distribuido) y
// desaparecen, luego arde el tronco. Fuego realista con curl noise.
// =============================================================================

#include "Scene.h"
#include <memory>

std::unique_ptr<Scene> createTreeScene();
