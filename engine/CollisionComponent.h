#pragma once

#include <bitset>
#include <unordered_map>
#include <cstdint>

#include "MathUtil.h"
#include <SFML/Graphics/Sprite.hpp>
#include <SFML/Graphics/Texture.hpp>

namespace ECSEngine {

/**
 * @brief Collision data for one entity: what it is, which sides touch something, and its boxes.
 *
 * Plain data. CollisionLateSystem fills previousCollisionBox, and CollisionSystem reads the
 * boxes and sets the contact flags.
 */
struct CollisionComponent {
    // Each flag is one bit (": 1") and "= 0" makes it off by default
    uint8_t staticEntity : 1 = 0;  // CollisionSystem never moves it, but it still blocks others
    uint8_t doesDamage : 1 = 0;    // Not handled by any system yet
    // The four contact flags describe the last CollisionSystem run: 1 if the entity was pushed
    // out of another box from that side
    uint8_t onGround : 1 = 0;
    uint8_t onCeiling : 1 = 0;
    uint8_t onLeftWall : 1 = 0;
    uint8_t onRightWall : 1 = 0;
    // The bounding box for collision detection, as an offset from the entity's loc
    Rect collisionBox{};
    // The previous bounding box for collision detection, in world position (offset + loc)
    Rect previousCollisionBox{};
};

}  // namespace ECSEngine
