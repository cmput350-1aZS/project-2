#pragma once

#include "Keyboard.h"

namespace ECSEngine {

// Tuning values. They have not been tuned yet
const float kFallMultiplier = 2.0;      // Pull is multiplied by this while falling (velocity.y > 0)
const float kFastFallMultiplier = 1.5;  // Extra multiplier while falling and Down is held
const float kWallSlideSpeed = 100.0f;   // Fastest downward velocity.y allowed on a wall slide

template <typename... Components>
class GravitySystem : public System<Components...> {
public:
    const std::string& GetName() const override {
        static std::string name("GravitySystem");
        return name;
    }

    /**
     * @brief Pulls entities down by changing their velocity.
     *
     * Runs after InputSystem and before MovementSystem. For every valid entity that has both a
     * MovementComponent and a GravityComponent, adds acceleration (pixels per second squared)
     * times the duration of the last frame in seconds (engine->GetFrameTime()) to velocity.
     * x is added as is. For y the pull is changed first:
     *  - While falling (velocity.y > 0) it is multiplied by kFallMultiplier, so falling is
     *    faster than rising.
     *  - While falling and Down is held (needs an InputComponent) it is also multiplied by
     *    kFastFallMultiplier.
     *  - While falling and pushing into a wall (the entity has an InputComponent and a
     *    CollisionComponent, the wall flag on one side is set and the key for that side is
     *    held), velocity.y is capped at kWallSlideSpeed after the update. The wall flags are
     *    last frame's, because CollisionSystem runs later.
     * disabledTimer is not used yet.
     *
     * @param engine Gives access to the entity manager and the frame time.
     * @return kContinue, so the engine keeps running.
     */
    tSystemReturn Run(EngineView<Components...>* engine) override {
        auto& em = engine->GetEntityManager();

        // Skip the gaps left by removed entities, and entities missing either component
        for (auto& e : em) {
            auto id = e.GetID();
            if (em.IsValid(id)) {
                if (em.template HasComponent<MovementComponent>(id) &&
                    em.template HasComponent<GravityComponent>(id)) {
                    auto& movementComponent = em.template GetComponent<MovementComponent>(id);
                    auto& gravityComponent = em.template GetComponent<GravityComponent>(id);

                    // Acceleration is per second, so scale by the seconds the last frame took.
                    // This changes velocity only; MovementSystem turns velocity into position
                    float frameTime = engine->GetFrameTime();
                    movementComponent.velocity.x += gravityComponent.acceleration.x * frameTime;

                    // pull is this frame's downward acceleration. It starts as the entity's own
                    // value and the falling rules below can make it stronger
                    float pull = gravityComponent.acceleration.y;

                    // Set to true below when the entity pushes into a wall. It is used after the
                    // speed is updated, so it carries the decision down to the cap
                    bool sliding = false;

                    // Screen y points down, so a positive velocity.y means falling
                    if (movementComponent.velocity.y > 0) {
                        pull = pull * kFallMultiplier;

                        // Only entities with an InputComponent (the player) can fast fall or
                        // wall slide. Entities without one, like stars, skip this block
                        if (em.template HasComponent<InputComponent>(id)) {
                            auto& inputComponent = em.template GetComponent<InputComponent>(id);
                            // Holding Down while falling pulls harder still
                            if (inputComponent.keydown[Key::Down]) {
                                pull = pull * kFastFallMultiplier;
                            }

                            // The wall flags live in CollisionComponent
                            if (em.template HasComponent<CollisionComponent>(id)) {
                                auto& collisionComponent =
                                    em.template GetComponent<CollisionComponent>(id);
                                // Pushing into a wall = a wall touches that side AND the key for
                                // that side is held. Pushing away from it does not count
                                if (collisionComponent.onLeftWall &&
                                    inputComponent.keydown[Key::Left]) {
                                    sliding = true;
                                }

                                if (collisionComponent.onRightWall &&
                                    inputComponent.keydown[Key::Right]) {
                                    sliding = true;
                                }
                            }
                        }
                    }
                    movementComponent.velocity.y += pull * frameTime;

                    // Wall slide: do not fall faster than kWallSlideSpeed. This has to come
                    // after the += above, which is what adds the speed
                    if (sliding && movementComponent.velocity.y > kWallSlideSpeed) {
                        movementComponent.velocity.y = kWallSlideSpeed;
                    }

                    // While sliding, play the "wall" sound, but only start it again once the
                    // last play has finished, or it would restart every frame
                    if (sliding) {
                        auto& wallSound = engine->GetSoundManager().GetSound("wall");
                        if (wallSound.getStatus() != sf::Sound::Status::Playing) {
                            wallSound.play();
                        }
                    }
                }
            }
        }
        return kContinue;
    }
};

}  // namespace ECSEngine
