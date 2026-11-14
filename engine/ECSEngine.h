#pragma once

#include <algorithm>
#include <vector>

#include "CameraComponent.h"
#include "CollisionComponent.h"
#include "EntityManager.h"
#include "InputComponent.h"
#include "LocationComponent.h"
#include "MathUtil.h"
#include "ScoreComponent.h"
#include "SoundManager.h"
#include "SpawnComponent.h"
#include "SpriteComponent.h"
#include "SpriteManager.h"
#include "SystemManager.h"
#include "WindowManager.h"
#include <SFML/Graphics.hpp>
#include <SFML/System/Clock.hpp>
#include <SFML/Window.hpp>

namespace ECSEngine {

/**
 * @brief What a system is allowed to see of the engine: the managers and the frame time.
 *
 * Systems receive an EngineView*, not the ECSEngine itself, so they can read and change the
 * game's data but cannot start or stop the engine loop.
 *
 * @tparam Components The component types the game uses.
 */
template <typename... Components>
class EngineView {
public:
    virtual SoundManager& GetSoundManager() = 0;
    virtual SpriteManager& GetSpriteManager() = 0;
    virtual EntityManager<Components...>& GetEntityManager() = 0;
    virtual WindowManager& GetWindowManager() = 0;

    /**
     * @brief Gives the duration of the last frame.
     *
     * Pure virtual (= 0): EngineView has no clock, so ECSEngine supplies the value.
     *
     * @return Seconds the previous frame took. It is 0 until the first frame has been timed.
     */
    virtual float GetFrameTime() = 0;
};

/**
 * @brief Owns the managers and runs the game loop.
 *
 * Each frame it handles window and keyboard events, measures the frame time, then runs every
 * system in the order set in SystemManager.
 *
 * @tparam Components The component types the game uses.
 */
template <typename... Components>
class ECSEngine : public EngineView<Components...> {
public:
    /**
     * @brief Creates the engine and its window.
     *
     * @param width The window width in pixels.
     * @param height The window height in pixels.
     * @param name The window title.
     */
    ECSEngine(unsigned int width, unsigned int height, const std::string& name);
    ~ECSEngine();

    /**
     * @brief Runs the game loop until the window is closed or a system returns kExit.
     */
    void Run();

    SoundManager& GetSoundManager() override { return mSoundManager; }

    SpriteManager& GetSpriteManager() override { return mSpriteManager; }

    EntityManager<Components...>& GetEntityManager() override { return mEntityManager; }

    WindowManager& GetWindowManager() override { return mWindowManager; }

    SystemManager<Components...>& GetSystemManager() { return mSystemManager; }

    /** @copydoc EngineView::GetFrameTime */
    float GetFrameTime() override { return mFrameTime; }

private:
    /**
     * @brief Handles window and keyboard events for this frame.
     *
     * A key press or release sets or clears that key's bit in keydown of every InputComponent.
     *
     * @return False if the window was closed, so the loop should stop; true otherwise.
     */
    bool ProcessEvents();
    void DebugDraw();

    EntityManager<Components...> mEntityManager;
    SystemManager<Components...> mSystemManager;
    SpriteManager mSpriteManager;
    SoundManager mSoundManager;
    WindowManager mWindowManager;

    float mFrameTime = 0.0f;  // Seconds the previous frame took; set once per frame in Run()
    sf::Clock mClock;         // Restarted every frame, so its reading is the frame's duration

    // For debugging - pause and step the engine
    bool running, step;
    int debug = 0;
};

template <typename... Components>
ECSEngine<Components...>::ECSEngine(unsigned int width, unsigned int height,
                                    const std::string& name)
    : mWindowManager(width, height, name) {}

template <typename... Components>
ECSEngine<Components...>::~ECSEngine() {}

template <typename... Components>
void ECSEngine<Components...>::Run() {
    // Set fps to 60
    mWindowManager.GetWindow()->setFramerateLimit(60);

    // Restart once before the loop so the time spent loading is not counted in frame 1
    mClock.restart();
    while (ProcessEvents()) {
        // restart() returns the time since the last restart, which is the previous frame's
        // duration, and starts timing the next frame
        mFrameTime = std::min(mClock.restart().asSeconds(), 1.0f / 30.0f);
        // Run every system in order; one returning kExit stops the game
        auto keepRunning = mSystemManager.Run(this);
        if (!keepRunning) {
            break;
        }

        if (debug) {
            DebugDraw();
        }
        mWindowManager.GetWindow()->display();
    }
}

template <typename... Components>
bool ECSEngine<Components...>::ProcessEvents() {
    auto window = mWindowManager.GetWindow();

    // pollEvent() hands back the waiting events one at a time until none are left
    while (auto event = window->pollEvent()) {
        // The window's close button was clicked: tell Run() to stop
        if (event->is<sf::Event::Closed>()) {
            return false;
        }

        // getIf returns a pointer if the event is a key press, otherwise nothing
        auto pressed = event->getIf<sf::Event::KeyPressed>();

        if (pressed) {
            if (pressed->scancode == sf::Keyboard::Scan::D) {
                debug = !debug;
            }
            // Every entity that listens to the keyboard gets the key marked as down
            for (Entity e : mEntityManager) {
                if (mEntityManager.IsValid(e.GetID())) {
                    if (mEntityManager.template HasComponent<InputComponent>(e.GetID())) {
                        auto& name =
                            mEntityManager.template GetComponent<InputComponent>(e.GetID());
                        name.keydown.set(static_cast<size_t>(pressed->scancode), true);
                    }
                }
            }
        }

        auto released = event->getIf<sf::Event::KeyReleased>();

        // Same loop for a key release, clearing the key's bit instead of setting it
        if (released) {
            for (Entity e : mEntityManager) {
                if (mEntityManager.IsValid(e.GetID())) {
                    if (mEntityManager.template HasComponent<InputComponent>(e.GetID())) {
                        auto& name =
                            mEntityManager.template GetComponent<InputComponent>(e.GetID());
                        name.keydown.set(static_cast<size_t>(released->scancode), false);
                    }
                }
            }
        }
    }
    return true;
}

template <typename... Components>
void ECSEngine<Components...>::DebugDraw() {
    for (Entity e : mEntityManager) {
        if (!mEntityManager.IsValid(e.GetID()) ||
            !mEntityManager.template HasComponent<CollisionComponent>(e.GetID()) ||
            !mEntityManager.template HasComponent<LocationComponent>(e.GetID())) {
            continue;
        }

        auto& collision = mEntityManager.template GetComponent<CollisionComponent>(e.GetID());
        auto& location = mEntityManager.template GetComponent<LocationComponent>(e.GetID());
        Rect box{Point2D(collision.collisionBox.topLeft.x + location.loc.x,
                         collision.collisionBox.topLeft.y + location.loc.y),
                 collision.collisionBox.width, collision.collisionBox.height};

        if (collision.staticEntity) {
            mWindowManager.DebugFrameRect(box, 255, 0, 0);
        } else {
            mWindowManager.DebugFrameRect(box, 0, 255, 0);
        }
    }
}

}  // namespace ECSEngine
