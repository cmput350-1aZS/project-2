#pragma once

#include "Keyboard.h"

namespace ECSEngine {

// Tuning values. They are applied once per frame and have not been tuned yet
const float kMaxRunVelocity = 15;        // Fastest left or right run speed
const float kDashVelocity = 50;          // Speed set by a dash
const float kJumpVelocity = 25;          // Upward speed set by a jump
const float kGroundRunAcceleration = 5;  // Speed gained per frame by a held arrow key on the ground
const float kAirRunAcceleration = 4;     // Same, in the air
const float kDashInterval = 25;          // Frames that must pass between two dashes

template <typename... Components>
class InputSystem : public System<Components...> {
public:
    const std::string& GetName() const override {
        static std::string name("InputSystem");
        return name;
    }

    /**
     * @brief Turns the keys the player holds into velocity changes.
     *
     * Runs after CollisionLateSystem and before GravitySystem. For every valid entity that has
     * an InputComponent, a MovementComponent and a CollisionComponent:
     *  - Right / Left add / subtract an acceleration (kGroundRunAcceleration on the ground,
     *    kAirRunAcceleration in the air), then velocity.x is clamped to +-kMaxRunVelocity.
     *  - Z, on the frame it is first pressed and while onGround, sets velocity.y to
     *    -kJumpVelocity (screen y points down, so up is negative).
     *  - C, on the frame it is first pressed and at least kDashInterval frames after the last
     *    dash, sets velocity.x to -kDashVelocity if Left is held, otherwise kDashVelocity, and
     *    resets lastDash to 0. lastDash counts frames and is incremented once per frame.
     *
     * "First pressed" means down in keydown and not down in lastKeydown. At the end of the frame
     * keydown is copied into lastKeydown for the next frame. The constants are applied once per
     * frame (not per second) and have not been tuned.
     *
     * @param engine Gives access to the entity manager.
     * @return kContinue, so the engine keeps running.
     */
    tSystemReturn Run(EngineView<Components...>* engine) override {
        auto& em = engine->GetEntityManager();

        for (auto& e : em) {
            auto id = e.GetID();
            if (em.IsValid(id)) {
                if (em.template HasComponent<InputComponent>(id) &&
                    em.template HasComponent<MovementComponent>(id) &&
                    em.template HasComponent<CollisionComponent>(id)) {
                    auto& inputComponent = em.template GetComponent<InputComponent>(id);
                    auto& movementComponent = em.template GetComponent<MovementComponent>(id);
                    auto& collisionComponent = em.template GetComponent<CollisionComponent>(id);

                    // The player speeds up more slowly in the air than on the ground
                    float acceleration;
                    if (collisionComponent.onGround) {
                        acceleration = kGroundRunAcceleration;
                    } else {
                        acceleration = kAirRunAcceleration;
                    }

                    // Holding Right speeds up to the right
                    if (inputComponent.keydown[Key::Right]) {
                        movementComponent.velocity.x += acceleration;
                    }

                    // Holding Left speeds up to the left (negative x)
                    if (inputComponent.keydown[Key::Left]) {
                        movementComponent.velocity.x -= acceleration;
                    }

                    // Two clamps, one per direction, so holding a key cannot speed up forever
                    if (movementComponent.velocity.x > kMaxRunVelocity) {
                        movementComponent.velocity.x = kMaxRunVelocity;
                    }

                    if (movementComponent.velocity.x < -kMaxRunVelocity) {
                        movementComponent.velocity.x = -kMaxRunVelocity;
                    }

                    // Jump only on the frame Z is first pressed, and only from the ground.
                    // Screen y points down, so up is negative; gravity later pulls it back down
                    if (inputComponent.keydown[Key::Z] && !inputComponent.lastKeydown[Key::Z] &&
                        collisionComponent.onGround) {
                        movementComponent.velocity.y = -kJumpVelocity;
                    }

                    // lastDash counts frames since the last dash. It lives in the component
                    // because a local variable would reset to 0 every frame
                    inputComponent.lastDash += 1;
                    // Dash only on a fresh C press, and only after the cooldown has passed
                    if (inputComponent.keydown[Key::C] &&
                        inputComponent.lastDash >= kDashInterval &&
                        !inputComponent.lastKeydown[Key::C]) {
                        // Dash left if Left is held, otherwise right. Using else means only one
                        // of the two lines runs, so the right dash cannot overwrite the left
                        if (inputComponent.keydown[Key::Left]) {
                            movementComponent.velocity.x = -kDashVelocity;
                        } else {
                            movementComponent.velocity.x = kDashVelocity;
                        }
                        // Start the cooldown again
                        inputComponent.lastDash = 0;
                    }

                    // Remember this frame's keys, so next frame can tell a new press from a
                    // key that is still held. This must stay after the checks above
                    inputComponent.lastKeydown = inputComponent.keydown;
                }
            }
        }
        return kContinue;
    }
};

}  // namespace ECSEngine
