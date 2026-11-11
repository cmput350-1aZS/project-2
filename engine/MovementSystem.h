#pragma once
namespace ECSEngine {

template <typename... Components>
class MovementSystem : public System<Components...> {
public:
    const std::string& GetName() const override {
        static std::string name("MovementSystem");
        return name;
    }

    /**
     * @brief Moves entities by their velocity.
     *
     * Runs after GravitySystem and before CollisionSystem. For every valid entity that has both
     * a MovementComponent and a LocationComponent, adds velocity (pixels per second) times the
     * duration of the last frame in seconds (engine->GetFrameTime()) to loc.
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
                    em.template HasComponent<LocationComponent>(id)) {
                    auto& movementComponent = em.template GetComponent<MovementComponent>(id);
                    auto& locationComponent = em.template GetComponent<LocationComponent>(id);

                    // Velocity is per second, so a step is velocity * seconds since last frame.
                    // "+=" because the position changes by a small step from where it is now
                    float frameTime = engine->GetFrameTime();
                    locationComponent.loc.x += movementComponent.velocity.x * frameTime;
                    locationComponent.loc.y += movementComponent.velocity.y * frameTime;
                }
            }
        }

        return kContinue;
    }
};

}  // namespace ECSEngine
