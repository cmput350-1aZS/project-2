#pragma once

#include <bitset>
#include <unordered_map>
#include <string>

#include "CameraComponent.h"
#include "EntityManager.h"
#include "MathUtil.h"
#include "SpriteManager.h"
#include "SystemManager.h"
#include <SFML/Graphics/Sprite.hpp>
#include <SFML/Graphics/Texture.hpp>

#include "Entity.h"
#include "LocationComponent.h"
#include "SpriteComponent.h"
#include "WindowManager.h"
#include <SFML/Graphics/RenderWindow.hpp>


namespace ECSEngine {

template <typename... Components>
class SpriteSystem : public System<Components...> {
public:
    const std::string& GetName() const override {
        static const std::string name("SpriteSystem");
        return name;
    }

    tSystemReturn Run(EngineView<Components...>* engine) override {
        auto& entities = engine->GetEntityManager();
        auto& spriteManager = engine->GetSpriteManager();
        auto& windowManager = engine->GetWindowManager();

        auto window = windowManager.GetWindow();

        if (!window || !window->isOpen()) {
            return kContinue;
        }

        for (const Entity& entity : entities) {
            EntityID id = entity.GetID();

            if (!entities.IsValid(id) ||
                !entities.template HasComponent<SpriteComponent>(id)) {
                continue;
            }

            const auto& spriteComponent =
                entities.template GetComponent<SpriteComponent>(id);

            Point2D basePosition(0.0f, 0.0f);

            if (entities.template HasComponent<LocationComponent>(id)) {
                basePosition =
                    entities.template GetComponent<LocationComponent>(id).loc;
            }

            Point2D drawPosition(
                basePosition.x + spriteComponent.bounds.topLeft.x,
                basePosition.y + spriteComponent.bounds.topLeft.y
            );

            if (spriteComponent.worldSpace) {
                drawPosition = windowManager.WorldToWindow(drawPosition);
            }

            sf::Sprite sprite =
                spriteManager.GetSprite(spriteComponent.textureID);

            sf::IntRect sourceRect = sprite.getTextureRect();

            if (sourceRect.size.x <= 0 || sourceRect.size.y <= 0) {
                continue;
            }

            float scaleX =
                spriteComponent.bounds.width /
                static_cast<float>(sourceRect.size.x);

            float scaleY =
                spriteComponent.bounds.height /
                static_cast<float>(sourceRect.size.y);

            if (spriteComponent.worldSpace) {
                const float worldScale = windowManager.GetWorldScale();

                if (worldScale <= 0.0f) {
                    continue;
                }

                scaleX /= worldScale;
                scaleY /= worldScale;
            }

            sprite.setPosition({drawPosition.x, drawPosition.y});
            sprite.setScale({scaleX, scaleY});

            window->draw(sprite);
        }

        window->display();

        return kContinue;
    }
};

}  // namespace ECSEngine