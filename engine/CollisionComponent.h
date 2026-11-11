#pragma once

// AI disclosure: Claude Code, used as a tutor. I typed the changes myself. The AI
// explained that ": 1" is a bit width, not a value, and named the syntax ": 1 = 0"
// for a default value on a bitfield (C++20) and "{}" for the two Rect members.
// Prompt that started this: "Im confused, so moiving on to GameComponents, im trying
// to work on CollisionComponent. But which lecture notes..."
// Quality: the defaults only fix uninitialized memory. The AI's claim that "most
// entities move" was wrong (it is not in the spec); 0 is a safe default, not a
// prediction. [CONFIRM: not compiled yet]

#include <bitset>
#include <unordered_map>

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
