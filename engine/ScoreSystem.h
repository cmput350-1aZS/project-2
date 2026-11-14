#pragma once

#include <algorithm>
#include <iterator>
#include <string>

#include "Entity.h"
#include "ScoreComponent.h"
#include "SpriteComponent.h"
#include "SystemManager.h"
#include "EntityManager.h"

namespace ECSEngine {

template <typename... Components>
class ScoreSystem : public System<Components...> {
public:
    const std::string& GetName() const override {
        static const std::string name("ScoreSystem");
        return name;
    }

    tSystemReturn Run(EngineView<Components...>* engine) override {
        auto& entities = engine->GetEntityManager();

        const EntityID entityCount =
            static_cast<EntityID>(
                std::distance(entities.cbegin(), entities.cend())
            );

        for (const Entity& entity : entities) {
            EntityID id = entity.GetID();

            if (!entities.IsValid(id) ||
                !entities.HasComponent<ScoreComponent>(id)) {
                continue;
            }

            auto& scoreComponent =
                entities.GetComponent<ScoreComponent>(id);

            const int score = std::clamp(scoreComponent.score, 0, 999);

            const int digits[3] = {
                score / 100,
                (score / 10) % 10,
                score % 10
            };

            for (int i = 0; i < 3; ++i) {
                const EntityID digitEntity = scoreComponent.digitID[i];

                if (digitEntity >= entityCount ||
                    !entities.IsValid(digitEntity) ||
                    !entities.HasComponent<SpriteComponent>(digitEntity)) {
                    continue;
                }

                auto& spriteComponent =
                    entities.GetComponent<SpriteComponent>(digitEntity);

                spriteComponent.textureID =
                    scoreComponent.digitTextureID[digits[i]];
            }
        }

        return kContinue;
    }
};

}  // namespace ECSEngine