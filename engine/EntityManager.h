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

// Owns every entity and every component in the engine. Components is the list of component types
// the game uses; each type gets its own ComponentStorage.
//  - An EntityID is the entity's position in an internal vector. IDs are never reused, so a removed
//    entity leaves a gap that stays invalid. Check IsValid() before using an ID you did not just
//    get from CreateEntity().
//  - A reference returned by GetComponent() or GetComponentStorage() points into a vector inside
//    the manager. It stays valid until the next AddComponent() of the same type, which may grow
//    the vector and move its contents. 
template <typename... Components>
class EntityManager {
public:
    // Stored in an entity's slot table when it has no component of that type
    static constexpr size_t kNullComponent = std::numeric_limits<size_t>::max();

    // Creates an entity called name with no components and returns its ID. IDs count up from 0.
    // Callers must keep the ID: it is the only way to refer to the entity afterwards
    [[nodiscard]] EntityID CreateEntity(const std::string& name) {
        auto newId = mEntities.size();
        Entity newEntity(newId, name);
        mEntities.push_back(newEntity);
        mValid.push_back(true);

        std::array<size_t, sizeof...(Components)> row;
        row.fill(kNullComponent);
        mSlots.push_back(row);

        return newId;
    }

    // Removes every component the entity has, then marks the entity invalid. The ID is not
    // reused and the entity's position stays in the iteration order as an invalid gap.
    // Removing an entity that is already removed does nothing
    void RemoveEntity(EntityID entity) {
        assert(entity < mEntities.size());
        (RemoveIfPresent<Components>(entity), ...);
        mValid[entity] = false;
    }

    // True if the entity exists, false if it was removed. Callers that loop over the entities use
    // this to skip the gaps left by RemoveEntity()
    bool IsValid(EntityID entity) const {
        assert(entity < mEntities.size());
        return mValid[entity];
    }

    // Gives the entity a copy of component. T must be one of the Components types. Reuses a slot
    // freed by RemoveComponent() if there is one, otherwise adds to the end of the T storage.
    // The entity must not already have a T: this is not checked, and adding twice leaves the old
    // component stuck in the storage with no entity pointing to it.
    // May invalidate references to T components from GetComponent()/GetComponentStorage()
    template <typename T>
    void AddComponent(EntityID entity, T component) {
        assert(entity < mEntities.size());
        auto& storage = std::get<ComponentStorage<T>>(mStorages);

        if (!storage.mFreeList.empty()) {
            auto slot = storage.mFreeList.back();
            storage.mFreeList.pop_back();
            storage.mStorage[slot] = component;
            storage.mValid[slot] = true;
            mSlots[entity][IndexOf<T>()] = slot;

        } else {
            auto newPosition = storage.mStorage.size();
            storage.mStorage.push_back(component);
            mSlots[entity][IndexOf<T>()] = newPosition;
            storage.mValid.push_back(true);
        }
    }

    // Takes the T away from the entity. The entity must have one (asserted in debug builds).
    // The component's slot is marked invalid and reused by a later AddComponent<T>(). Its data is
    // not erased and the vector does not shrink, so earlier references stay safe to read but must
    // not be used as the entity's component any more
    template <typename T>
    void RemoveComponent(EntityID entity) {
        assert(entity < mEntities.size());
        assert(HasComponent<T>(entity));
        auto& storage = std::get<ComponentStorage<T>>(mStorages);
        auto slot = mSlots[entity][IndexOf<T>()];
        storage.mValid[slot] = false;
        storage.mFreeList.push_back(slot);
        mSlots[entity][IndexOf<T>()] = kNullComponent;
    }

    // True if the entity currently has a component of type T
    template <typename T>
    bool HasComponent(EntityID entity) const {
        assert(entity < mEntities.size());
        if (mSlots[entity][IndexOf<T>()] != kNullComponent) {
            return true;
        }

        return false;
    }

    // Returns the entity's T so it can be read or changed in place.
    // The reference is owned by the manager, not the caller, and is
    // only valid until the next AddComponent<T>(); do not store it
    template <typename T>
    T& GetComponent(EntityID entity) {
        assert(entity < mEntities.size());
        assert(HasComponent<T>(entity));
        auto& storage = std::get<ComponentStorage<T>>(mStorages);
        auto slot = mSlots[entity][IndexOf<T>()];
        return storage.mStorage[slot];
    }

    // Returns the storage holding every T in the engine, for systems that loop over all of one
    // component type. An item's position in mStorage is its slot, not the entity ID, and removed
    // items stay in the vector: skip positions where mValid[position] is false. Same lifetime rule
    // as GetComponent(): valid until the next AddComponent<T>()
    template <typename T>
    ComponentStorage<T>& GetComponentStorage() {
        return std::get<ComponentStorage<T>>(mStorages);
    }

    // Iteration visits every entity position in ID order
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

    // A table of where each entity's components are stored
    // Row n is entity n. There is one column per component type
    // A cell holds a position in that type's list, or kNullComponent if
    // the entity has no component of that type.
    // Example: mSlots[2][1] == 1 means entity 2's second component type is at position 1 of its
    // list
    std::vector<std::array<size_t, sizeof...(Components)>> mSlots;

    // The column of type T in the slot table: T's position in the Components list. Returns
    // kNullComponent if T is not in the list
    template <typename T>
    static constexpr size_t IndexOf() {
        std::array<bool, sizeof...(Components)> matches = {std::is_same_v<T, Components>...};

        for (size_t i = 0; i < matches.size(); ++i) {
            if (matches[i]) {
                return i;
            }
        }

        return kNullComponent;
    }

    // RemoveComponent<T>() that does nothing if the entity has no T. Used by RemoveEntity()
    template <typename T>
    void RemoveIfPresent(EntityID entity) {
        if (HasComponent<T>(entity)) {
            RemoveComponent<T>(entity);
        }
    }
};

}  // namespace ECSEngine
