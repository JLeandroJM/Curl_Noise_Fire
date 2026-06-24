#pragma once
// =============================================================================
// CampfireScene.h - Escena "hero": fogata de leña con fuego realista (curl noise)
// =============================================================================

#include "Scene.h"
#include <memory>

std::unique_ptr<Scene> createCampfireScene();
