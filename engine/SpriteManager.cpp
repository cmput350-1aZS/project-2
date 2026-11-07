#include "SpriteManager.h"

#include <stdexcept>
#include <utility>

namespace ECSEngine {

SpriteID SpriteManager::RegisterTexture(const std::string& texturePath, const Rect& sourceRect) {
    auto textureIt = mTextures.find(texturePath);

    if (textureIt == mTextures.end()) {
        sf::Texture texture;

        if (!texture.loadFromFile(texturePath)) {
            throw std::runtime_error(texturePath + " failed to load");
        }

        textureIt = mTextures.emplace(texturePath, std::move(texture)).first;
    }

    sf::IntRect sfmlRect(
        {static_cast<int>(sourceRect.topLeft.x), static_cast<int>(sourceRect.topLeft.y)},
        {static_cast<int>(sourceRect.width), static_cast<int>(sourceRect.height)}
    );
    mSprites.emplace_back(textureIt->second, sfmlRect);

    // return 0;
    return mSprites.size() - 1;
    
}

sf::Sprite& SpriteManager::GetSprite(SpriteID id) {
    // static sf::Texture dummyTexture;
    // static sf::Sprite dummy(dummyTexture);
    // return dummy;
    if (id >= mSprites.size()) {
        throw std::out_of_range("Invalid sprite ID");
    }
    return mSprites[id];
}

}  // namespace ECSEngine
