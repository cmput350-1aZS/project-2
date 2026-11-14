#include <fstream>
#include <iostream>
#include <string>
#include <unordered_map>

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
    // AI disclosure (sounds): Claude Code, used as a tutor. I typed every line. The AI gave the
    // RegisterSound signature, that it throws if a file cannot load, the plan (four calls), the
    // name-to-file table, and the call shape RegisterSound(gResourcePath + "example.ogg",
    // "example") after my attempts failed. Prompts: "go, start with sounds", "im confused, for
    // these "load" code, do i need the system and compoennets?". Quality: my first versions had
    // an empty call, a variable name that clashed with the later "sm", and two file names pasted
    // into one path, which would have thrown at startup. [CONFIRM: not compiled or run by me yet]
    auto& soundManager = engine.GetSoundManager();
    soundManager.RegisterSound(gResourcePath + "sfx_jump.ogg", "jump");
    soundManager.RegisterSound(gResourcePath + "footstep_grass_003.ogg", "land");
    soundManager.RegisterSound(gResourcePath + "sfx_gem.ogg", "star");
    soundManager.RegisterSound(gResourcePath + "footstep_snow_001.ogg", "wall");

    // Load maps
    // AI disclosure (maps): Claude Code, used as a tutor. I typed both calls. The AI gave the
    // LoadMap signature and said sky.map goes first, because later drawing covers earlier
    // drawing. That only matters if SpriteSystem draws in creation order, and SpriteSystem is
    // still a stub. Prompts: "ok so next load maps?", "idk". Quality: my first attempt passed the
    // namespace instead of the engine, left out the file name and had only one call.
    LoadMap(engine, gResourcePath, "sky.map");
    LoadMap(engine, gResourcePath, "world.map");

    // Place player
    // AI disclosure (player): Claude Code, used as a tutor. I typed every line. The AI gave the
    // plan (one entity, one AddComponent per component), the API facts (CreateEntity is
    // [[nodiscard]]; braces for plain structs, parentheses for GravityComponent and
    // InputComponent, which have constructors), the sprite sheet facts (128x128 cells, drawn at
    // 64x64), a measured collision box (I chose the simple 64x64 one instead), the shape
    // GravityComponent(Point2D(x, y)) and the start-position hint (floor top at y = 640). The
    // start position (704, 500) and gravity 1000 are rough guesses, not tuned. Prompts: "go",
    // "why are we setting a vuale of 1000 when we are just spaawning the player", "im confused,
    // we are makign the collision box of the player right? but hwow do wek now waht vlaues",
    // "ok im confused about this". Quality: my slips were an undeclared sprite variable, a name
    // typo (collisiion), an empty collision box and a respawn point of (0, 0) instead of the
    // start. [CONFIRM: not compiled or run by me yet]
    auto& em = engine.GetEntityManager();
    ECSEngine::EntityID player = em.CreateEntity("player");
    em.AddComponent(player, ECSEngine::LocationComponent{{704, 500}});
    em.AddComponent(player, ECSEngine::MovementComponent{{0, 0}});
    em.AddComponent(player, ECSEngine::GravityComponent(ECSEngine::Point2D(0, 1000)));

    auto& texture = engine.GetSpriteManager();
    ECSEngine::SpriteID charId = texture.RegisterTexture(
        gResourcePath + "spritesheet-characters-default.png", ECSEngine::Rect{{0, 0}, 128, 128});
    em.AddComponent(player,
                    ECSEngine::SpriteComponent(charId, ECSEngine::Rect{{0, 0}, 64, 64}, true));

    ECSEngine::CollisionComponent collision;
    collision.collisionBox = {{0, 0}, 64, 64};
    em.AddComponent(player, collision);

    em.AddComponent(player, ECSEngine::InputComponent(player));
    em.AddComponent(player, ECSEngine::CameraFollower(player));
    em.AddComponent(player, ECSEngine::RespawnComponent{{704, 500}});
    em.AddComponent(player, ECSEngine::CameraShake(0, {0, 0}));

    // Setup score component
    // AI disclosure (score): Claude Code, used as a tutor. I typed every line. The AI gave the
    // design (what ScoreComponent holds, three digit entities drawn in screen space, their ids
    // kept in digitID because Entity has no lookup by name), the digit picture positions
    // (x = 832, y = 832 - 64 * digit, read off spritesheet-tiles-default.png by looking at the
    // image; not tested in the game), the digit-0 line that I turned into the loop, the "{}"
    // after the variable name, the digit positions on screen (x = 10 + 40 * k, y = 10), the two
    // lines that put scoreComponent on a "score" entity after the loops (AddComponent stores a
    // copy), and the .0f fix for the ambiguous Point2D(float, int) error. That error was the
    // AI's mistake: it told me y = 10 needed no cast. Prompts: "next setup score compenet?",
    // "what even is the score? i thoguth its jus ta singular number which is called the score",
    // "ok i dont get what this is: EntityID digitID[3];", "wdym 3 entieis are drawon on the
    // screen". Quality: my slips were a missing "{}", assigning to the whole array, a loop that
    // ran 2 times instead of 3, and the same digit position in every pass. ScoreSystem is still a
    // stub, so the display has never changed. [CONFIRM: not compiled or run by me yet]
    ECSEngine::ScoreComponent scoreComponent = {};
    scoreComponent.score = 0;

    for (int i = 0; i < 10; i++) {
        scoreComponent.digitTextureID[i] = texture.RegisterTexture(
            gResourcePath + "spritesheet-tiles-default.png",
            ECSEngine::Rect{{832.0f, static_cast<float>(832 - 64 * i)}, 64, 64});
    }

    for (int k = 0; k < 3; k++) {
        scoreComponent.digitID[k] = em.CreateEntity("digit");
        em.AddComponent(scoreComponent.digitID[k],
                        ECSEngine::LocationComponent{{static_cast<float>(10 + 40 * k), 10.0f}});
        em.AddComponent(scoreComponent.digitID[k],
                        ECSEngine::SpriteComponent(scoreComponent.digitTextureID[0],
                                                   ECSEngine::Rect{{0, 0}, 40, 40}, false));
    }

    ECSEngine::EntityID scoreEntity = em.CreateEntity("score");
    em.AddComponent(scoreEntity, scoreComponent);

    // Setup Camera
    // AI disclosure (camera): Claude Code, used as a tutor. I typed every line. The AI gave the
    // field list, the values (zoom 1.0, position = player start, speed 0.1, which is a guess) and
    // both exclusion rectangles, which it wrote out in full after I asked how to get the numbers.
    // They are percentages of the 1024 x 768 window: hard = x 102.4 to 921.6, soft = x 307.2 to
    // 716.8. The meaning of the two rectangles and of position is the AI's reading of vague
    // comments in CameraComponent.h, not something the PDF states. Prompts: "im confused, what
    // is hardexclucsion", "how am i supseod to know what value to put". Quality: I first wrote
    // "{{}}" (an empty rectangle) twice. CameraSystem is still a stub, so the camera has never
    // moved. [CONFIRM: not compiled or run by me yet]
    ECSEngine::CameraComponent cameraComponent = {};
    cameraComponent.worldUnitsPerPixel = 1.0f;
    cameraComponent.position = {704.0f, 500.0f};
    cameraComponent.speed = 0.1f;
    cameraComponent.hardExclusion = {{102.4f, 0.0f}, 819.2f, 768.0f};
    cameraComponent.softExclusion = {{307.2f, 0.0f}, 409.6f, 768.0f};

    ECSEngine::EntityID cameraEntity = em.CreateEntity("camera");
    em.AddComponent(cameraEntity, cameraComponent);

    // Place extra star
    // AI disclosure (extra star): Claude Code, used as a tutor. I typed every line. The AI gave
    // the plan, the star picture rectangle {{640, 320}, 64, 64} (the same one LoadMap uses), the
    // position (640, 192), which is on top of the wall at column 10, row 4 of world.map, that
    // the collision box is not static, the name starCollision (collision is already taken) and
    // the three fixes to my sprite line (the entity, SpriteComponent, starId). Prompts: "is this
    // a test?", "can you write it" (the AI declined and pointed me to my earlier lines). Quality:
    // I first passed the picture id where the entity belongs and re-added the location instead
    // of the collision component. Whether the wall jump can reach the star is untested.
    // [CONFIRM: not compiled or run by me yet]
    ECSEngine::SpriteID starId = texture.RegisterTexture(
        gResourcePath + "spritesheet-tiles-default.png", ECSEngine::Rect{{640, 320}, 64, 64});

    ECSEngine::EntityID starEntity = em.CreateEntity("star");
    em.AddComponent(starEntity, ECSEngine::LocationComponent{{640, 192}});
    em.AddComponent(starEntity,
                    ECSEngine::SpriteComponent(starId, ECSEngine::Rect{{0, 0}, 64, 64}, true));

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
