#pragma once

#include <bitset>
#include <unordered_map>

#include "MathUtil.h"
#include <SFML/Graphics/Sprite.hpp>
#include <SFML/Graphics/Texture.hpp>

namespace ECSEngine {

// location for respawning
struct RespawnComponent {
    Point2D loc;
};

}  // namespace ECSEngine
