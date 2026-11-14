#pragma once

namespace ECSEngine {

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
                    movementComponent.velocity.y += gravityComponent.acceleration.y * frameTime;
                }
            }
        }
        return kContinue;
    }
};

}  // namespace ECSEngine
