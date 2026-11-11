#pragma once

#include <array>
#include <cassert>
#include <iostream>
#include <limits>
#include <type_traits>
#include <vector>

#include "Component.h"
#include "Entity.h"

namespace ECSEngine {

/**
 * @brief Owns every entity and every component in the engine.
 *
 * Each type in Components gets its own ComponentStorage.
 *  - An EntityID is the entity's position in an internal vector. IDs are never reused, so a
 *    removed entity leaves a gap that stays invalid. Check IsValid() before using an ID you did
 *    not just get from CreateEntity().
 *  - A reference returned by GetComponent() or GetComponentStorage() points into a vector inside
 *    the manager. It stays valid until the next AddComponent() of the same type, which may grow
 *    the vector and move its contents.
 *
 * @tparam Components The component types the game uses.
 */
template <typename... Components>
class EntityManager {
public:
    /** @brief Stored in an entity's slot table when it has no component of that type. */
    static constexpr size_t kNullComponent = std::numeric_limits<size_t>::max();

    /**
     * @brief Creates an entity with no components.
     *
     * Callers must keep the returned ID: it is the only way to refer to the entity afterwards.
     *
     * @param name The entity's name.
     * @return The new entity's ID. IDs count up from 0.
     */
    [[nodiscard]] EntityID CreateEntity(const std::string& name) {
        // The next free position is the new ID
        auto newId = mEntities.size();
        Entity newEntity(newId, name);
        mEntities.push_back(newEntity);
        mValid.push_back(true);

        // One new row in the slot table; every cell starts as "no component of this type"
        std::array<size_t, sizeof...(Components)> row;
        row.fill(kNullComponent);
        mSlots.push_back(row);

        return newId;
    }

    /**
     * @brief Removes every component the entity has, then marks the entity invalid.
     *
     * The ID is not reused and the entity's position stays in the iteration order as an invalid
     * gap. Removing an entity that is already removed does nothing.
     *
     * @param entity The entity to remove.
     */
    void RemoveEntity(EntityID entity) {
        assert(entity < mEntities.size());
        // Remove each component type the entity has (the fold expression runs once per type)
        (RemoveIfPresent<Components>(entity), ...);
        mValid[entity] = false;
    }

    /**
     * @brief Tells whether an entity still exists.
     *
     * Callers that loop over the entities use this to skip the gaps left by RemoveEntity().
     *
     * @param entity The entity to check.
     * @return True if the entity exists, false if it was removed.
     */
    bool IsValid(EntityID entity) const {
        assert(entity < mEntities.size());
        return mValid[entity];
    }

    /**
     * @brief Gives the entity a copy of a component.
     *
     * Reuses a slot freed by RemoveComponent() if there is one, otherwise adds to the end of the
     * T storage. The entity must not already have a T: this is not checked, and adding twice
     * leaves the old component stuck in the storage with no entity pointing to it. May
     * invalidate references to T components from GetComponent() and GetComponentStorage().
     *
     * @tparam T One of the Components types.
     * @param entity The entity that receives the component.
     * @param component The value to copy into the storage.
     */
    template <typename T>
    void AddComponent(EntityID entity, T component) {
        assert(entity < mEntities.size());
        auto& storage = std::get<ComponentStorage<T>>(mStorages);

        if (!storage.mFreeList.empty()) {
            // A slot freed earlier is available: reuse it so the storage does not grow
            auto slot = storage.mFreeList.back();
            storage.mFreeList.pop_back();
            storage.mStorage[slot] = component;
            storage.mValid[slot] = true;
            mSlots[entity][IndexOf<T>()] = slot;

        } else {
            // No free slot: append to the end and remember where it went
            auto newPosition = storage.mStorage.size();
            storage.mStorage.push_back(component);
            mSlots[entity][IndexOf<T>()] = newPosition;
            storage.mValid.push_back(true);
        }
    }

    /**
     * @brief Takes the T away from the entity.
     *
     * The entity must have one (asserted in debug builds). The component's slot is marked invalid
     * and reused by a later AddComponent<T>(). Its data is not erased and the vector does not
     * shrink, so earlier references stay safe to read but must not be used as the entity's
     * component any more.
     *
     * @tparam T The component type to remove.
     * @param entity The entity that loses the component.
     */
    template <typename T>
    void RemoveComponent(EntityID entity) {
        assert(entity < mEntities.size());
        assert(HasComponent<T>(entity));
        auto& storage = std::get<ComponentStorage<T>>(mStorages);
        // Find where this entity's T is stored
        auto slot = mSlots[entity][IndexOf<T>()];
        // Hide the slot, offer it to the next AddComponent<T>(), and forget it for this entity
        storage.mValid[slot] = false;
        storage.mFreeList.push_back(slot);
        mSlots[entity][IndexOf<T>()] = kNullComponent;
    }

    /**
     * @brief Tells whether an entity has a component of type T.
     *
     * @tparam T The component type to look for.
     * @param entity The entity to check.
     * @return True if the entity currently has a T.
     */
    template <typename T>
    bool HasComponent(EntityID entity) const {
        assert(entity < mEntities.size());
        if (mSlots[entity][IndexOf<T>()] != kNullComponent) {
            return true;
        }

        return false;
    }

    /**
     * @brief Gives access to the entity's T so it can be read or changed in place.
     *
     * The entity must have a T (asserted in debug builds).
     *
     * @tparam T The component type to get.
     * @param entity The entity that owns the component.
     * @return A reference to the stored component. The manager owns it, not the caller, and it
     *         is only valid until the next AddComponent<T>(); do not store it.
     */
    template <typename T>
    T& GetComponent(EntityID entity) {
        assert(entity < mEntities.size());
        assert(HasComponent<T>(entity));
        auto& storage = std::get<ComponentStorage<T>>(mStorages);
        // The slot table says which position in the T storage belongs to this entity
        auto slot = mSlots[entity][IndexOf<T>()];
        return storage.mStorage[slot];
    }

    /**
     * @brief Gives access to the storage holding every T in the engine.
     *
     * For systems that loop over all of one component type. An item's position in mStorage is its
     * slot, not the entity ID, and removed items stay in the vector: skip positions where
     * mValid[position] is false.
     *
     * @tparam T The component type whose storage is wanted.
     * @return A reference to the storage. Same lifetime rule as GetComponent(): valid until the
     *         next AddComponent<T>().
     */
    template <typename T>
    ComponentStorage<T>& GetComponentStorage() {
        return std::get<ComponentStorage<T>>(mStorages);
    }

    /** @brief Iteration visits every entity position in ID order, removed ones included. */
    using tEntity = std::vector<Entity>;
    using const_iterator = tEntity::const_iterator;
    const_iterator cbegin() const { return mEntities.begin(); }
    const_iterator cend() const { return mEntities.end(); }
    const_iterator begin() { return mEntities.begin(); }
    const_iterator end() { return mEntities.end(); }

private:
    std::vector<Entity> mEntities;
    std::vector<bool> mValid;
    std::tuple<ComponentStorage<Components>...> mStorages;

    /**
     * @brief A table of where each entity's components are stored.
     *
     * Row n is entity n. There is one column per component type. A cell holds a position in that
     * type's list, or kNullComponent if the entity has no component of that type. Example:
     * mSlots[2][1] == 1 means entity 2's second component type is at position 1 of its list.
     */
    std::vector<std::array<size_t, sizeof...(Components)>> mSlots;

    /**
     * @brief Finds the column of type T in the slot table.
     *
     * @tparam T The component type to look up.
     * @return T's position in the Components list, or kNullComponent if T is not in the list.
     */
    template <typename T>
    static constexpr size_t IndexOf() {
        // matches[i] is true where Components[i] is the same type as T
        std::array<bool, sizeof...(Components)> matches = {std::is_same_v<T, Components>...};

        for (size_t i = 0; i < matches.size(); ++i) {
            if (matches[i]) {
                return i;
            }
        }

        return kNullComponent;
    }

    /**
     * @brief RemoveComponent<T>() that does nothing if the entity has no T. Used by RemoveEntity().
     *
     * @tparam T The component type to remove if present.
     * @param entity The entity to remove it from.
     */
    template <typename T>
    void RemoveIfPresent(EntityID entity) {
        if (HasComponent<T>(entity)) {
            RemoveComponent<T>(entity);
        }
    }
};

}  // namespace ECSEngine
