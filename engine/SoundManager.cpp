#include "SoundManager.h"
#include <stdexcept>

namespace ECSEngine {

void SoundManager::RegisterSound(const std::string& soundPath, const std::string& soundName) {
    sf::SoundBuffer buffer;

    if (!buffer.loadFromFile(soundPath)) {
        throw std::runtime_error(soundPath + " failed to load");
    }

    auto [bufferIt, inserted] =
        mSoundBuffers.emplace(soundName, std::move(buffer));

    if (!inserted) {
        throw std::runtime_error(soundName + " is already registered");
    }

    mSounds.emplace(soundName, sf::Sound(bufferIt->second));
}

sf::Sound& SoundManager::GetSound(const std::string& soundName) {
    // static sf::SoundBuffer dummyBuffer;
    // static sf::Sound dummy(dummyBuffer);
    // return dummy;
    auto it = mSounds.find(soundName);

    if (it == mSounds.end()) {
        throw std::runtime_error(soundName + " not found");
    }
    return it->second;
}

}  // namespace ECSEngine
