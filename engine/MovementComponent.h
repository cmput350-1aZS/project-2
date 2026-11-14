#pragma once

#include <bitset>
#include <unordered_map>

#include "MathUtil.h"
#include <SFML/Graphics/Sprite.hpp>
#include <SFML/Graphics/Texture.hpp>

namespace ECSEngine {

struct MovementComponent {
    Point2D velocity;  // Velocity in pixels per second
};

}  // namespace ECSEngine
