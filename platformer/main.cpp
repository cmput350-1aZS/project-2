#include <fstream>
#include <iostream>
#include <string>
#include <unordered_map>

// These structs must be declared before the system headers below, which name them directly
#include "GravityComponent.h"
#include "MovementComponent.h"
#include "RespawnComponent.h"

#include "CameraSystem.h"
#include "CollisionComponent.h"
#include "CollisionLateSystem.h"
#include "CollisionSystem.h"
#include "ECSEngine.h"
#include "EntityManager.h"
#include "GravitySystem.h"
#include "InputComponent.h"
#include "InputSystem.h"
#include "LocationComponent.h"
#include "MathUtil.h"
#include "MovementSystem.h"
#include "ScoreComponent.h"
#include "ScoreSystem.h"
#include "SpawnComponent.h"
#include "SpawnSystem.h"
#include "SpriteComponent.h"
#include "SpriteManager.h"
#include "SpriteSystem.h"

std::string gResourcePath = "../assets/";
#define COMPONENTS                                                                             \
    ECSEngine::LocationComponent, ECSEngine::MovementComponent, ECSEngine::CollisionComponent, \
        ECSEngine::SpriteComponent, ECSEngine::SpawnComponent, ECSEngine::CameraComponent,     \
        ECSEngine::CameraFollower, ECSEngine::InputComponent, ECSEngine::GravityComponent,     \
        ECSEngine::CameraShake, ECSEngine::ScoreComponent, ECSEngine::RespawnComponent
using Engine = ECSEngine::ECSEngine<COMPONENTS>;

void LoadMap(Engine& engine, const std::string& path, const std::string& file) {
    std::string fullPath = path + file;
    std::string word;
    int number;
    std::ifstream in(fullPath);

    if (!in) {
        std::cerr << "cannot open " << fullPath << "\n";
        return;
    }

    in >> word >> number;
    std::cout << word << " " << number << "\n";

    std::string texturePath;
    // Source rectangle: which part of the spritesheet picture to cut out
    // sx, sy = left and top edge in pixels; sw, sh = width and height in pixels
    float sx, sy, sw, sh;
    // Collision box, only present in type 2 dictionaries
    // bx, by = offset from the sprite's top-left corner; bw, bh = box width and height
    int bx, by, bw, bh;

    std::unordered_map<std::string, ECSEngine::SpriteID> sprites;
    std::unordered_map<std::string, ECSEngine::Rect> boxes;

    while (in >> word) {
        if (word == "map") {
            break;
        }

        if (number == 2) {
            in >> texturePath >> sx >> sy >> sw >> sh >> bx >> by >> bw >> bh;
            boxes[word] = ECSEngine::Rect{{static_cast<float>(bx), static_cast<float>(by)},
                                          static_cast<float>(bw),
                                          static_cast<float>(bh)};
        } else {
            in >> texturePath >> sx >> sy >> sw >> sh;
        }

        ECSEngine::Rect rect{{sx, sy}, sw, sh};
        ECSEngine::SpriteID id =
            engine.GetSpriteManager().RegisterTexture(path + texturePath, rect);
        sprites[word] = id;
    }
    // origin x, origin y, tile width, tile height
    float ox, oy, tw, th;
    // grid width, grid height
    int gw, gh;

    in >> word >> ox >> oy >> word >> tw >> th >> word >> gw >> gh;

    for (int row = 0; row < gh; row++) {
        in >> word;

        for (int col = 0; col < gw; col++) {
            char letter = word[col];

            if (letter == '.') {
                continue;
            }

            auto& em = engine.GetEntityManager();
            ECSEngine::EntityID tile = em.CreateEntity("tile");
            float x = ox + (col * tw);
            float y = oy + (row * th);
            em.AddComponent(tile, ECSEngine::LocationComponent{{x, y}});

            if (letter == 'S') {
                ECSEngine::Rect starRect{{640, 320}, 64, 64};
                ECSEngine::SpriteID starId = engine.GetSpriteManager().RegisterTexture(
                    path + "spritesheet-tiles-default.png", starRect);

                em.AddComponent(tile,
                                ECSEngine::SpawnComponent{tile, "star", starId, 60, 120, 1000});
                continue;
            }

            ECSEngine::SpriteID spriteId = sprites[std::string(1, letter)];
            em.AddComponent(tile, ECSEngine::SpriteComponent(
                                      {spriteId, ECSEngine::Rect{{0, 0}, tw, th}, true}));

            if (number == 2) {
                ECSEngine::CollisionComponent collision;
                collision.staticEntity = 1;
                collision.collisionBox = boxes[std::string(1, letter)];
                em.AddComponent(tile, collision);
            }
        }
    }
}

int main(int argc, char* argv[]) {
    if (argc >= 3 && argv[1] == std::string("-path")) {
        gResourcePath = argv[2];
    }
    std::cout << "Usage: " << argv[0] << " [-path resource_path]\n";
    std::cout << "Using resource path: " << gResourcePath << "\n";

    // Engine test code
    Engine engine(1024, 768, "Test Engine");

    // Load sounds
    // Each sound is registered once under a short name. Systems play it later with GetSound(name). 
    auto& soundManager = engine.GetSoundManager();
    soundManager.RegisterSound(gResourcePath + "sfx_jump.ogg", "jump");
    soundManager.RegisterSound(gResourcePath + "footstep_grass_003.ogg", "land");
    soundManager.RegisterSound(gResourcePath + "sfx_gem.ogg", "star");
    soundManager.RegisterSound(gResourcePath + "footstep_snow_001.ogg", "wall");

    // Load maps
    LoadMap(engine, gResourcePath, "sky.map");
    LoadMap(engine, gResourcePath, "world.map");

    // Place player
    // The player is one entity made of components. Location is the top-left corner in world
    // pixels, starting a little above the floor (the floor's top edge is y = 640), so gravity
    // drops it onto the floor. It starts at rest (velocity 0). Gravity is an acceleration in
    // pixels per second squared; it stays on the entity for the whole game, it is not a one-off
    // push. The value is a guess to be tuned.
    auto& em = engine.GetEntityManager();
    ECSEngine::EntityID player = em.CreateEntity("player");
    em.AddComponent(player, ECSEngine::LocationComponent{{704, 500}});
    em.AddComponent(player, ECSEngine::MovementComponent{{0, 0}});
    em.AddComponent(player, ECSEngine::GravityComponent(ECSEngine::Point2D(0, 1000)));

    // The picture is the first 128 x 128 cell of the characters sheet. SpriteComponent draws it
    // at 64 x 64 (one tile) from the entity's location, in world space.
    auto& texture = engine.GetSpriteManager();
    ECSEngine::SpriteID charId = texture.RegisterTexture(
        gResourcePath + "spritesheet-characters-default.png", ECSEngine::Rect{{0, 0}, 128, 128});
    em.AddComponent(player,
                    ECSEngine::SpriteComponent(charId, ECSEngine::Rect{{0, 0}, 64, 64}, true));

    // The collision box is an offset from the location: here the whole 64 x 64 square. 
    ECSEngine::CollisionComponent collision;
    collision.collisionBox = {{0, 0}, 64, 64};
    em.AddComponent(player, collision);

    // InputComponent makes the entity listen to the keyboard
    // CameraFollower names the entity the camera tracks. RespawnComponent stores where to
    // return after dying (the start point). CameraShake starts with no shake.
    em.AddComponent(player, ECSEngine::InputComponent(player));
    em.AddComponent(player, ECSEngine::CameraFollower(player));
    em.AddComponent(player, ECSEngine::RespawnComponent{{704, 500}});
    em.AddComponent(player, ECSEngine::CameraShake(0, {0, 0}));

    // Setup score component
    // The score is a number (score) plus what is needed to draw it: a picture for each digit
    // 0-9 (digitTextureID) and the three entities that show the digits on screen (digitID).
    ECSEngine::ScoreComponent scoreComponent = {};
    scoreComponent.score = 0;

    // The digits 0-9 sit in one column of the tiles sheet, 64 px apart, with 0 at the bottom:
    // digit d is at x = 832, y = 832 - 64 * d. Slot d of digitTextureID holds the picture of d.
    for (int i = 0; i < 10; i++) {
        scoreComponent.digitTextureID[i] = texture.RegisterTexture(
            gResourcePath + "spritesheet-tiles-default.png",
            ECSEngine::Rect{{832.0f, static_cast<float>(832 - 64 * i)}, 64, 64});
    }

    // Three digit entities in the top-left corner, 40 px apart, each showing the picture of 0.
    // The sprites are in screen space (worldSpace = false), so the location is in window
    // pixels. Their ids are saved in digitID because Entity has no lookup by name.
    for (int k = 0; k < 3; k++) {
        scoreComponent.digitID[k] = em.CreateEntity("digit");
        em.AddComponent(scoreComponent.digitID[k],
                        ECSEngine::LocationComponent{{static_cast<float>(10 + 40 * k), 10.0f}});
        em.AddComponent(scoreComponent.digitID[k],
                        ECSEngine::SpriteComponent(scoreComponent.digitTextureID[0],
                                                   ECSEngine::Rect{{0, 0}, 40, 40}, false));
    }

    // The component must be attached after the loops: AddComponent stores a copy, so it has to
    // be filled in first.
    ECSEngine::EntityID scoreEntity = em.CreateEntity("score");
    em.AddComponent(scoreEntity, scoreComponent);

    // Setup Camera
    // No zoom (1 world unit per pixel). position starts at the player's start point, and speed
    // is the tween speed for the slow follow
    ECSEngine::CameraComponent cameraComponent = {};
    cameraComponent.worldUnitsPerPixel = 1.0f;
    cameraComponent.position = {704.0f, 500.0f};
    cameraComponent.speed = 0.1f;
    // The zones are rectangles in screen pixels, as fractions of the 1024 x 768 window. The
    // player must stay inside hardExclusion (x from 10% = 102.4 to 90% = 921.6); outside it
    // the camera follows directly. softExclusion is the middle 40% (x from 30% = 307.2 to
    // 70% = 716.8): between the two the camera slowly moves the player back toward the middle.
    cameraComponent.hardExclusion = {{102.4f, 0.0f}, 819.2f, 768.0f};
    cameraComponent.softExclusion = {{307.2f, 0.0f}, 409.6f, 768.0f};

    ECSEngine::EntityID cameraEntity = em.CreateEntity("camera");
    em.AddComponent(cameraEntity, cameraComponent);

    // Place extra star
    // One star placed by hand on top of the left wall (column 10, row 4 of world.map, so its
    // top edge is y = 256 and a 64 px star sits at y = 192). The player can only reach it by
    // wall jumping, which tests the wall jump. The picture is the star cell of the tiles sheet,
    // the same one the spawner uses.
    ECSEngine::SpriteID starId = texture.RegisterTexture(
        gResourcePath + "spritesheet-tiles-default.png", ECSEngine::Rect{{640, 320}, 64, 64});

    ECSEngine::EntityID starEntity = em.CreateEntity("star");
    em.AddComponent(starEntity, ECSEngine::LocationComponent{{640, 192}});
    em.AddComponent(starEntity,
                    ECSEngine::SpriteComponent(starId, ECSEngine::Rect{{0, 0}, 64, 64}, true));

    // Not static, so it counts as a dynamic item (a collectable) rather than a wall.
    ECSEngine::CollisionComponent starCollision;
    starCollision.collisionBox = {{0, 0}, 64, 64};
    em.AddComponent(starEntity, starCollision);

    // Add all systems
    auto& sm = engine.GetSystemManager();
    sm.AddSystem(std::make_unique<ECSEngine::CollisionLateSystem<COMPONENTS>>());
    sm.AddSystem(std::make_unique<ECSEngine::InputSystem<COMPONENTS>>());
    sm.AddSystem(std::make_unique<ECSEngine::GravitySystem<COMPONENTS>>());
    sm.AddSystem(std::make_unique<ECSEngine::MovementSystem<COMPONENTS>>());
    sm.AddSystem(std::make_unique<ECSEngine::CollisionSystem<COMPONENTS>>());
    sm.AddSystem(std::make_unique<ECSEngine::ScoreSystem<COMPONENTS>>());
    sm.AddSystem(std::make_unique<ECSEngine::CameraSystem<COMPONENTS>>());
    sm.AddSystem(std::make_unique<ECSEngine::SpriteSystem<COMPONENTS>>());
    sm.AddSystem(std::make_unique<ECSEngine::SpawnSystem<COMPONENTS>>());

    engine.Run();
    return 0;
}
