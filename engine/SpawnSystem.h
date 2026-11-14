
#pragma once

#include <cmath>
#include <random>
#include <string>
#include <vector>

#include "CollisionComponent.h"
#include "Entity.h"
#include "EntityManager.h"
#include "LocationComponent.h"
#include "MovementComponent.h"
#include "SpawnComponent.h"
#include "SpriteComponent.h"
#include "SystemManager.h"

namespace ECSEngine {

template <typename... Components>
class SpawnSystem : public System<Components...> {
public:
    const std::string& GetName() const override {
        static const std::string name("SpawnSystem");
        return name;
    }

    tSystemReturn Run(EngineView<Components...>* engine) override {
        auto& entities = engine->GetEntityManager();

        static std::mt19937 generator(std::random_device{}());

        static std::uniform_real_distribution<float> angleDistribution(
            0.0f, 6.28318530718f
        );

        static std::uniform_real_distribution<float> speedDistribution(
            100.0f, 180.0f
        );

        std::vector<EntityID> spawnerIDs;

        // Collect spawners before creating entities because CreateEntity()
        // can invalidate the entity iterator.
        for (const Entity& entity : entities) {
            const EntityID id = entity.GetID();

            if (entities.IsValid(id) &&
                entities.template HasComponent<SpawnComponent>(id)) {
                spawnerIDs.push_back(id);
            }
        }

        for (EntityID id : spawnerIDs) {
            if (!entities.IsValid(id)) {
                continue;
            }

            auto& spawn = entities.template GetComponent<SpawnComponent>(id);

            // No more entities should be spawned by this spawner.
            if (spawn.totalSpawns <= 0) {
                continue;
            }

            // Count down the spawn timer.
            if (spawn.timeToNextSpawn > 0) {
                --spawn.timeToNextSpawn;
            }

            if (spawn.timeToNextSpawn > 0) {
                continue;
            }

            // The spawner must have a valid owner with a location.
            const EntityID ownerID = spawn.entityID;

            if (!entities.IsValid(ownerID) ||
                !entities.template HasComponent<LocationComponent>(ownerID)) {
                continue;
            }

            const Point2D spawnLocation =
                entities.template GetComponent<LocationComponent>(ownerID).loc;

            // Create the new entity.
            const EntityID newEntity =
                entities.CreateEntity(spawn.spawnType);

            // Add its location.
            LocationComponent location{};
            location.loc = spawnLocation;
            entities.template AddComponent<LocationComponent>(newEntity, location);

            // Add its sprite.
            SpriteComponent sprite{};
            sprite.textureID = spawn.spawnTextureID;
            sprite.bounds = {{0.0f, 0.0f}, 32.0f, 32.0f};
            sprite.worldSpace = true;
            entities.template AddComponent<SpriteComponent>(newEntity, sprite);

            // Give it a random direction and speed.
            const float angle = angleDistribution(generator);
            const float speed = speedDistribution(generator);

            MovementComponent movement{};
            movement.velocity = {
                std::cos(angle) * speed,
                std::sin(angle) * speed
            };

            entities.template AddComponent<MovementComponent>(
                newEntity, movement
            );

            // Add its collision box.
            CollisionComponent collision{};
            collision.staticEntity = false;
            collision.doesDamage = false;
            collision.onGround = false;
            collision.onCeiling = false;
            collision.onLeftWall = false;
            collision.onRightWall = false;
            collision.collisionBox = {
                {0.0f, 0.0f}, 32.0f, 32.0f
            };
            collision.previousCollisionBox = collision.collisionBox;

            entities.template AddComponent<CollisionComponent>(
                newEntity, collision
            );

            // Update the spawner for its next spawn.
            --spawn.totalSpawns;
            spawn.timeToNextSpawn = spawn.timeBetweenSpawns;
        }

        return kContinue;
    }
};

}  // namespace ECSEngine
