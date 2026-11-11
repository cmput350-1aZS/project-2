#include <cassert>
#include <iostream>

#include "EntityManager.h"

struct TestA {
    int value;
};
struct TestB {
    int value;
};

using Manager = ECSEngine::EntityManager<TestA, TestB>;

// removing an entity makes it invalid, leaves the others alone, and never reuses its ID
static void TestRemoveEntity() {
    Manager em;
    auto a = em.CreateEntity("a");
    auto b = em.CreateEntity("b");
    auto c = em.CreateEntity("c");

    em.RemoveEntity(a);
    assert(!em.IsValid(a));
    assert(em.IsValid(b));
    assert(em.IsValid(c));

    auto d = em.CreateEntity("d");
    assert(d == 3);
    assert(em.IsValid(d));

    em.RemoveEntity(a);  // removing twice does nothing
    assert(!em.IsValid(a));
}

// add, read, change in place, and keep the two component types apart
static void TestComponents() {
    Manager em;
    auto e1 = em.CreateEntity("e1");
    auto e2 = em.CreateEntity("e2");

    em.AddComponent(e1, TestA{5});
    assert(em.HasComponent<TestA>(e1));
    assert(!em.HasComponent<TestB>(e1));
    assert(!em.HasComponent<TestA>(e2));
    assert(em.GetComponent<TestA>(e1).value == 5);

    em.GetComponent<TestA>(e1).value = 9;  // the reference changes the stored component
    assert(em.GetComponent<TestA>(e1).value == 9);

    em.AddComponent(e1, TestB{7});
    assert(em.HasComponent<TestB>(e1));
    assert(em.GetComponent<TestA>(e1).value == 9);
    assert(em.GetComponent<TestB>(e1).value == 7);
}

// a slot freed by RemoveComponent is reused, so the storage does not grow
static void TestSlotReuse() {
    Manager em;
    auto e1 = em.CreateEntity("e1");
    auto e2 = em.CreateEntity("e2");
    em.AddComponent(e1, TestA{1});
    em.AddComponent(e2, TestA{2});

    auto& storage = em.GetComponentStorage<TestA>();
    assert(storage.mStorage.size() == 2);

    em.RemoveComponent<TestA>(e1);
    assert(!em.HasComponent<TestA>(e1));
    assert(!storage.mValid[0]);
    assert(storage.mFreeList.size() == 1);

    auto e3 = em.CreateEntity("e3");
    em.AddComponent(e3, TestA{3});
    assert(storage.mStorage.size() == 2);  // reused slot 0, nothing was added
    assert(storage.mValid[0]);
    assert(storage.mFreeList.empty());
    assert(em.GetComponent<TestA>(e3).value == 3);
    assert(em.GetComponent<TestA>(e2).value == 2);  // the neighbour is untouched
}

// removing an entity frees every component it had
static void TestRemoveEntityClearsComponents() {
    Manager em;
    auto e1 = em.CreateEntity("e1");
    em.AddComponent(e1, TestA{1});
    em.AddComponent(e1, TestB{2});

    em.RemoveEntity(e1);
    assert(!em.IsValid(e1));
    assert(!em.HasComponent<TestA>(e1));
    assert(!em.HasComponent<TestB>(e1));
    assert(!em.GetComponentStorage<TestA>().mValid[0]);
    assert(!em.GetComponentStorage<TestB>().mValid[0]);
    assert(em.GetComponentStorage<TestA>().mFreeList.size() == 1);
    assert(em.GetComponentStorage<TestB>().mFreeList.size() == 1);
}

// Test 6: the iterator visits every position, removed ones included, in ID order.
static void TestIteration() {
    Manager em;
    auto a = em.CreateEntity("a");
    auto b = em.CreateEntity("b");
    auto c = em.CreateEntity("c");
    em.RemoveEntity(b);

    size_t visited = 0;
    size_t validCount = 0;
    for (const auto& entity : em) {
        ++visited;
        if (em.IsValid(entity.GetID())) {
            ++validCount;
            assert(entity.GetID() == a || entity.GetID() == c);
        }
    }
    assert(visited == 3);
    assert(validCount == 2);
}

int main() {
    ECSEngine::EntityManager<TestA, TestB> em;

    auto first_id = em.CreateEntity("first");

    auto second_id = em.CreateEntity("second");

    assert(first_id == 0);
    assert(second_id == 1);
    assert(em.IsValid(first_id));
    assert(em.IsValid(second_id));

    TestRemoveEntity();
    TestComponents();
    TestSlotReuse();
    TestRemoveEntityClearsComponents();
    TestIteration();

    std::cout << "All EntityManager tests passed\n";
    return 0;
}
