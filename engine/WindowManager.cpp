#include "WindowManager.h"

#include <algorithm>
#include <cmath>
#include <cstdint>
#include <stdexcept>

namespace ECSEngine {

WindowManager::WindowManager(unsigned int width, unsigned int height, const std::string& title)
    : mWindow(std::make_shared<sf::RenderWindow>(sf::VideoMode({width, height}), title)),
      mWorldCenter(width / 2.0f, height / 2.0f),
      mWorldScale(1.0f) {}

std::shared_ptr<sf::RenderWindow> WindowManager::GetWindow() const { return mWindow; }

Rect WindowManager::GetWindowBounds() const {
    sf::Vector2u size = mWindow->getSize();
    return {{0, 0}, static_cast<float>(size.x), static_cast<float>(size.y)};
    // return Rect();
}

void WindowManager::DebugDrawRect(const Rect& r, int red, int green, int blue, int alpha) {
    Rect windowRect = WorldToWindow(r);
    sf::RectangleShape shape({windowRect.width, windowRect.height});

    shape.setPosition({windowRect.topLeft.x, windowRect.topLeft.y});
    shape.setFillColor(sf::Color(static_cast<std::uint8_t>(std::clamp(red, 0, 255)),
                                 static_cast<std::uint8_t>(std::clamp(green, 0, 255)),
                                 static_cast<std::uint8_t>(std::clamp(blue, 0, 255)),
                                 static_cast<std::uint8_t>(std::clamp(alpha, 0, 255))));

    mWindow->draw(shape);
}

void WindowManager::DebugFrameRect(const Rect& r, int red, int green, int blue, int alpha) {
    Rect windowRect = WorldToWindow(r);

    sf::RectangleShape shape({windowRect.width, windowRect.height});

    shape.setPosition({windowRect.topLeft.x, windowRect.topLeft.y});
    shape.setFillColor(sf::Color::Transparent);
    shape.setOutlineColor(sf::Color(static_cast<std::uint8_t>(std::clamp(red, 0, 255)),
                                    static_cast<std::uint8_t>(std::clamp(green, 0, 255)),
                                    static_cast<std::uint8_t>(std::clamp(blue, 0, 255)),
                                    static_cast<std::uint8_t>(std::clamp(alpha, 0, 255))));
    shape.setOutlineThickness(1.0f);

    mWindow->draw(shape);
}

Rect WindowManager::WindowToWorld(const Rect& rect) const {
    Point2D worldTopLeft = WindowToWorld(rect.topLeft);

    return {worldTopLeft, rect.width * mWorldScale, rect.height * mWorldScale};
}

Point2D WindowManager::WindowToWorld(const Point2D& pt) const {
    sf::Vector2u size = mWindow->getSize();

    return {(pt.x - size.x / 2.0f) * mWorldScale + mWorldCenter.x,
            (pt.y - size.y / 2.0f) * mWorldScale + mWorldCenter.y};

    // return Point2D();
}

Rect WindowManager::WorldToWindow(const Rect& rect) const {
    Point2D windowTopLeft = WorldToWindow(rect.topLeft);

    return {windowTopLeft, rect.width / mWorldScale, rect.height / mWorldScale};
    // return Rect();
}

Point2D WindowManager::WorldToWindow(const Point2D& pt) const {
    sf::Vector2u size = mWindow->getSize();

    return {(pt.x - mWorldCenter.x) / mWorldScale + size.x / 2.0f,
            (pt.y - mWorldCenter.y) / mWorldScale + size.y / 2.0f};
    // return Point2D();
}

void WindowManager::SetCamera(const Point2D& worldPt, const Point2D& screenPt) {
    sf::Vector2u size = mWindow->getSize();

    Point2D windowCenter(size.x / 2.0f, size.y / 2.0f);

    mWorldCenter = {worldPt.x - (screenPt.x - windowCenter.x) * mWorldScale,
                    worldPt.y - (screenPt.y - windowCenter.y) * mWorldScale};
}

Point2D WindowManager::GetWorldCenter() const {
    return mWorldCenter;
    // return Point2D();
}

float WindowManager::GetWorldScale() const {
    return mWorldScale;
    // return 1;
}

void WindowManager::SetWorldCenter(const Point2D& worldCenter) { mWorldCenter = worldCenter; }

void WindowManager::SetWorldScale(float worldUnitsPerPixel) {
    if (worldUnitsPerPixel <= 0.0f || !std::isfinite(worldUnitsPerPixel)) {
        throw std::invalid_argument("invalid world scale");
    }

    mWorldScale = worldUnitsPerPixel;
}

void WindowManager::SetWorldRect(const Rect& worldCameraRect) {
    sf::Vector2u size = mWindow->getSize();

    if (size.x == 0 || size.y == 0) {
        throw std::runtime_error("invalid camera window size");
    }

    mWorldScale = worldCameraRect.width / size.x;
    mWorldCenter = {worldCameraRect.topLeft.x + worldCameraRect.width / 2.0f,
                    worldCameraRect.topLeft.y + worldCameraRect.height / 2.0f};
}

float WindowManager::WindowToWorld(float x) const {
    // converts horizontal window coords -> world coords
    float windowCenterX = mWindow->getSize().x / 2.0f;
    return (x - windowCenterX) * mWorldScale + mWorldCenter.x;

    // return 0;
}

float WindowManager::WorldToWindow(float x) const {
    // converts horizontal worldcoords -> window coords
    float windowCenterX = mWindow->getSize().x / 2.0f;
    return (x - mWorldCenter.x) / mWorldScale + windowCenterX;
    // return 0;
}

}  // namespace ECSEngine
