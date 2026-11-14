#pragma once

#include "InputComponent.h"
#include "ScoreComponent.h"
#include <cmath>
#include "CameraShakeComponent.h"

namespace ECSEngine {

const float kBounceKeep = 0.7f;  // Share of speed a star keeps when it bounces
const float kShakeMinSpeed = 400.0f;  // Slowest hit (pixels per second) that shakes the screen
const float kShakeScale = 0.01f;      // Shake size in pixels per pixel-per-second of hit speed
const int kShakeFrames = 8;           // Frames a shake lasts

/**
 * @brief Tests whether two boxes share area.
 *
 * Boxes that only touch along an edge do not overlap.
 *
 * @param a The first box, in world coordinates.
 * @param b The second box, in world coordinates.
 * @return True if the boxes overlap, false otherwise.
 */
inline bool Overlaps(const Rect& a, const Rect& b) {
    // The boxes overlap unless one is entirely to one side of the other
    if (a.topLeft.x + a.width <= b.topLeft.x) {
        return false;  // a is entirely left of b
    } else if (a.topLeft.y + a.height <= b.topLeft.y) {
        return false;  // a is entirely above b
    } else if (a.topLeft.x >= b.topLeft.x + b.width) {
        return false;  // a is entirely right of b
    } else if (a.topLeft.y >= b.topLeft.y + b.height) {
        return false;  // a is entirely below b
    }

    return true;
}

template <typename... Components>

class CollisionSystem : public System<Components...> {
public:
    const std::string& GetName() const override {
        static std::string name("CollisionSystem");
        return name;
    }

    /**
     * @brief Pushes moving entities out of whatever they now overlap.
     *
     * Runs after MovementSystem. For every valid, non-static entity with a CollisionComponent
     * and a LocationComponent: clear the four contact flags, build its world box
     * (collisionBox + loc), then test it against every other such entity. On an overlap,
     * previousCollisionBox (saved by CollisionLateSystem before the move) tells which side
     * the entity came from: it was above (landing, onGround), below (onCeiling), left
     * (onRightWall) or right (onLeftWall) of the other box. The entity's loc is pushed so its
     * box touches the other box, and its velocity on that axis is set to 0 when it has a
     * MovementComponent. Static entities are never moved, but they still block others.
     *
     * @param engine Gives access to the entity manager.
     * @return kContinue, so the engine keeps running.
     */
    tSystemReturn Run(EngineView<Components...>* engine) override {
        auto& em = engine->GetEntityManager();
        for (auto& e : em) {
            auto id = e.GetID();
            if (em.IsValid(id)) {
                if (em.template HasComponent<CollisionComponent>(id) &&
                    em.template HasComponent<LocationComponent>(id)) {
                    auto& collisionComponent = em.template GetComponent<CollisionComponent>(id);
                    auto& locationComponent = em.template GetComponent<LocationComponent>(id);

                    // Static entities are never moved, so skip them as the moving one. They are
                    // still tested as "other" in the inner loop below, so they block others
                    if (collisionComponent.staticEntity) {
                        continue;
                    }

                    // This entity's box in world coordinates: collisionBox is an offset, so
                    // add loc to the corner and keep the size
                    Rect currentBox{
                        Point2D(
                            collisionComponent.collisionBox.topLeft.x + locationComponent.loc.x,
                            collisionComponent.collisionBox.topLeft.y + locationComponent.loc.y),
                        collisionComponent.collisionBox.width,
                        collisionComponent.collisionBox.height,
                    };

                    bool wasOnGround = collisionComponent.onGround;

                    // The contact flags describe this frame only, so clear them before testing
                    collisionComponent.onGround = 0;
                    collisionComponent.onCeiling = 0;
                    collisionComponent.onLeftWall = 0;
                    collisionComponent.onRightWall = 0;

                    for (auto& other : em) {
                        auto otherId = other.GetID();
                        // An entity cannot collide with itself
                        if (otherId == id) {
                            continue;
                        }

                        if (em.IsValid(otherId)) {
                            if (em.template HasComponent<CollisionComponent>(otherId) &&
                                em.template HasComponent<LocationComponent>(otherId)) {
                                auto& otherCollisionComponent =
                                    em.template GetComponent<CollisionComponent>(otherId);
                                auto& otherLocationComponent =
                                    em.template GetComponent<LocationComponent>(otherId);

                                // The other entity's box in world coordinates, built the same way
                                Rect otherBox{
                                    Point2D(otherCollisionComponent.collisionBox.topLeft.x +
                                                otherLocationComponent.loc.x,
                                            otherCollisionComponent.collisionBox.topLeft.y +
                                                otherLocationComponent.loc.y),
                                    otherCollisionComponent.collisionBox.width,
                                    otherCollisionComponent.collisionBox.height,
                                };

                                // On an overlap, the box from before the move tells which side the
                                // entity came from. The tests run in this order; the first true
                                // one is used
                                if (Overlaps(currentBox, otherBox)) {
                                    if (em.template HasComponent<InputComponent>(id) &&
                                        !otherCollisionComponent.staticEntity) {
                                        em.RemoveEntity(otherId);
                                        for (auto& s : em) {
                                            if (em.template HasComponent<ScoreComponent>(
                                                    s.GetID())) {
                                                auto& scoreComponent =
                                                    em.template GetComponent<ScoreComponent>(
                                                        s.GetID());
                                                scoreComponent.score += 1;
                                            }
                                        }
                                        engine->GetSoundManager().GetSound("star").play();
                                        continue;
                                    }

                                    if (collisionComponent.previousCollisionBox.topLeft.y +
                                            collisionComponent.previousCollisionBox.height <=
                                        otherBox.topLeft.y) {
                                        // It was above the other box, so it landed. The push
                                        // below puts its bottom edge on the other box's top:
                                        // loc.y = other top - box height - box offset. Then
                                        // velocity.y is set to 0 so it stops falling
                                        collisionComponent.onGround = 1;
                                        locationComponent.loc.y =
                                            otherBox.topLeft.y -
                                            collisionComponent.collisionBox.height -
                                            collisionComponent.collisionBox.topLeft.y;

                                        if (em.template HasComponent<MovementComponent>(id)) {
                                            auto& movementComponent =
                                                em.template GetComponent<MovementComponent>(id);

                                            if (em.template HasComponent<InputComponent>(id)) {
                                                movementComponent.velocity.y = 0;
                                            } else {
                                                movementComponent.velocity.y *=
                                                    -kBounceKeep;  // Not the player, so only stars
                                                                   // bounce
                                            }
                                        }
                                    } else if (collisionComponent.previousCollisionBox.topLeft.y >=
                                               otherBox.topLeft.y + otherBox.height) {
                                        // It was below the other box, so it hit its underside.
                                        // The push puts its top edge on the other box's bottom
                                        collisionComponent.onCeiling = 1;
                                        locationComponent.loc.y =
                                            otherBox.topLeft.y + otherBox.height -
                                            collisionComponent.collisionBox.topLeft.y;

                                        if (em.template HasComponent<MovementComponent>(id)) {
                                            auto& movementComponent =
                                                em.template GetComponent<MovementComponent>(id);

                                            if (em.template HasComponent<InputComponent>(id)) {
                                                movementComponent.velocity.y = 0;
                                            } else {
                                                movementComponent.velocity.y *=
                                                    -kBounceKeep;  // Not the player, so only stars
                                                                   // bounce
                                            }
                                        }

                                    } else if (collisionComponent.previousCollisionBox.topLeft.x +
                                                   collisionComponent.previousCollisionBox.width <=
                                               otherBox.topLeft.x) {
                                        // It was left of the other box, so a wall is on its right.
                                        // Same push as landing, with x and width instead of y
                                        // and height
                                        collisionComponent.onRightWall = 1;
                                        locationComponent.loc.x =
                                            otherBox.topLeft.x -
                                            collisionComponent.collisionBox.width -
                                            collisionComponent.collisionBox.topLeft.x;

                                        if (em.template HasComponent<MovementComponent>(id)) {
                                            auto& movementComponent =
                                                em.template GetComponent<MovementComponent>(id);
                                            if (em.template HasComponent<InputComponent>(id)) {
                                                movementComponent.velocity.x = 0;
                                            } else {
                                                movementComponent.velocity.x *=
                                                    -kBounceKeep;  // Not the player, so only stars
                                                                   // bounce
                                            }
                                        }
                                    } else if (collisionComponent.previousCollisionBox.topLeft.x >=
                                               otherBox.topLeft.x + otherBox.width) {
                                        // It was right of the other box, so a wall is on its left.
                                        // The push puts its left edge on the other box's right
                                        collisionComponent.onLeftWall = 1;
                                        locationComponent.loc.x =
                                            otherBox.topLeft.x + otherBox.width -
                                            collisionComponent.collisionBox.topLeft.x;
                                        if (em.template HasComponent<MovementComponent>(id)) {
                                            auto& movementComponent =
                                                em.template GetComponent<MovementComponent>(id);
                                            if (em.template HasComponent<InputComponent>(id)) {
                                                movementComponent.velocity.x = 0;
                                            } else {
                                                movementComponent.velocity.x *=
                                                    -kBounceKeep;  // Not the player, so only stars
                                                                   // bounce
                                            }
                                        }
                                    }
                                }
                            }
                        }
                    }

                    // The player touches the ground this frame but did not last frame: it
                    // just landed
                    if (em.template HasComponent<InputComponent>(id) &&
                        collisionComponent.onGround && !wasOnGround) {
                        engine->GetSoundManager().GetSound("land").play();
                    }
                }
            }
        }
        return kContinue;
    }
};

}  // namespace ECSEngine
