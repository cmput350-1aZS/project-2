#pragma once

#include <unordered_map>
#include <algorithm>
#include <string>
#include <random>

#include "LocationComponent.h"
#include "MathUtil.h"
#include "SpriteComponent.h"
#include "SystemManager.h"
#include <SFML/Graphics/Sprite.hpp>
#include <SFML/Graphics/Texture.hpp>

#include "CameraComponent.h"
#include "CameraFollowerComponent.h"
#include "CameraShakeComponent.h"
#include "Entity.h"
#include "EntityManager.h"
#include "LocationComponent.h"
#include "WindowManager.h"

namespace ECSEngine {

template <typename... Components>
class CameraSystem : public System<Components...> {
public:
    const std::string& GetName() const override {
        static const std::string name("CameraSystem");
        return name;
    }

    tSystemReturn Run(EngineView<Components...>* engine) override {
        auto& entities = engine->GetEntityManager();
        auto& windowManager = engine->GetWindowManager();

        CameraComponent* camera = nullptr;
        CameraShake* shake = nullptr;

        EntityID targetID = 0;
        bool foundFollower = false;

        for (const Entity& entity : entities) {
            const EntityID id = entity.GetID();

            if (!entities.IsValid(id)) {
                continue;
            }

            if (entities.template HasComponent<CameraComponent>(id)) {
                camera = &entities.template GetComponent<CameraComponent>(id);

                if (entities.template HasComponent<CameraShake>(id)) {
                    shake = &entities.template GetComponent<CameraShake>(id);
                }
            }

            if (entities.template HasComponent<CameraFollower>(id)) {
                targetID =
                    entities.template GetComponent<CameraFollower>(id).entityID;
                foundFollower = true;
            }
        }

        if (camera == nullptr || !foundFollower ||
            !entities.IsValid(targetID) ||
            !entities.template HasComponent<LocationComponent>(targetID)) {
            return kContinue;
        }

        if (camera->worldUnitsPerPixel <= 0.0f) {
            return kContinue;
        }

        auto window = windowManager.GetWindow();

        if (!window || !window->isOpen()) {
            return kContinue;
        }

        const float windowWidth =
            static_cast<float>(window->getSize().x);

        if (windowWidth <= 0.0f) {
            return kContinue;
        }

        windowManager.SetWorldScale(camera->worldUnitsPerPixel);

        const Point2D targetPosition =
            entities.template GetComponent<LocationComponent>(targetID).loc;

        const float screenX =
            windowManager.WorldToWindow(targetPosition.x);

        // The rectangles are defined in screen coordinates.
        const float hardLeft = camera->hardExclusion.topLeft.x;
        const float hardRight =
            camera->hardExclusion.topLeft.x +
            camera->hardExclusion.width;

        const float softLeft = camera->softExclusion.topLeft.x;
        const float softRight =
            camera->softExclusion.topLeft.x +
            camera->softExclusion.width;

        float desiredScreenX = screenX;
        bool hardFollow = false;
        bool softFollow = false;

        // Hard boundary: move the camera immediately.
        if (screenX < hardLeft) {
            desiredScreenX = hardLeft;
            hardFollow = true;
        } else if (screenX > hardRight) {
            desiredScreenX = hardRight;
            hardFollow = true;
        }
        // Soft boundary: smoothly move the camera.
        else if (screenX < softLeft) {
            desiredScreenX = softLeft;
            softFollow = true;
        } else if (screenX > softRight) {
            desiredScreenX = softRight;
            softFollow = true;
        }

        if (hardFollow || softFollow) {
            const float desiredCenterX =
                targetPosition.x -
                (desiredScreenX - windowWidth / 2.0f) *
                    camera->worldUnitsPerPixel;

            if (hardFollow) {
                camera->position.x = desiredCenterX;
            } else {
                const float smoothing =
                    std::clamp(camera->speed, 0.0f, 1.0f);

                camera->position.x +=
                    (desiredCenterX - camera->position.x) * smoothing;
            }
        }

        // Keep the camera's vertical position fixed.
        Point2D cameraCenter = camera->position;

        // Apply a changing shake offset while the shake is active.
        if (shake != nullptr && shake->frameLimit > 0) {
            static std::mt19937 generator(std::random_device{}());
            static std::uniform_real_distribution<float> shakeDistribution(
                -1.0f, 1.0f
            );

            cameraCenter.x +=
                shake->offset.x * shakeDistribution(generator);

            cameraCenter.y +=
                shake->offset.y * shakeDistribution(generator);

            --shake->frameLimit;
        }

        windowManager.SetWorldCenter(cameraCenter);

        return kContinue;
    }
};

}  // namespace ECSEngine