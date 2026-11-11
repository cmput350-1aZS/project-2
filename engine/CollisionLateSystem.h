#pragma once

// AI disclosure: Claude Code, used as a tutor. I typed the body of Run. The AI gave me:
// "auto& em = engine->GetEntityManager();", the method names GetID and IsValid (capital
// ID / I), the shape of em.template GetComponent<Type>(id), the left side
// "collisionComponent.previousCollisionBox.topLeft.x", and a plain-word list of the four
// Rect values. It also pointed out that my first y line read previousCollisionBox
// instead of collisionBox. Prompts: "im kinda stukc", "IDKKK",
// "i dont get it, i dont see whats wrong".
// Quality: checked by reading only. [CONFIRM: not compiled yet]

namespace ECSEngine {

template <typename... Components>
class CollisionLateSystem : public System<Components...> {
public:
    const std::string& GetName() const override {
        static std::string name("CollisionLateSystem");
        return name;
    }

    /**
     * @brief Saves each entity's box before it moves this frame.
     *
     * Runs first each frame, before MovementSystem. For every valid entity that has both a
     * CollisionComponent and a LocationComponent, stores collisionBox (an offset from the
     * entity) moved by loc as previousCollisionBox, a world-position box. CollisionSystem later
     * compares it with the new position to find which side was hit.
     *
     * @param engine Gives access to the entity manager.
     * @return kContinue, so the engine keeps running.
     */
    tSystemReturn Run(EngineView<Components...>* engine) override {
        auto& em = engine->GetEntityManager();

        // Skip the gaps left by removed entities, and entities missing either component
        for (auto& e : em) {
            auto id = e.GetID();
            if (em.IsValid(id)) {
                if (em.template HasComponent<CollisionComponent>(id) &&    em.template HasComponent<LocationComponent>(id)) {
                    auto& collisionComponent = em.template GetComponent<CollisionComponent>(id);
                    auto& locationComponent = em.template GetComponent<LocationComponent>(id);

                    // Add loc to the box offset to get the world position, and keep the size
                    collisionComponent.previousCollisionBox.topLeft.x = collisionComponent.collisionBox.topLeft.x + locationComponent.loc.x;
                    collisionComponent.previousCollisionBox.topLeft.y = collisionComponent.collisionBox.topLeft.y + locationComponent.loc.y;
                    collisionComponent.previousCollisionBox.width = collisionComponent.collisionBox.width;
                    collisionComponent.previousCollisionBox.height = collisionComponent.collisionBox.height;

                }
              
             
            }
        }
        return kContinue;
    }
};

}  // namespace ECSEngine
