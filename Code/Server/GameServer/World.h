#pragma once

#include "Entity.h"
#include "Component.h"
#include "ComponentPool.h"
#include "SystemManager.h"

class World {
public:
    World() : mNextEntityID(1) {
    }
    void Update(float deltaTime) { mSystemManager->Update(deltaTime); }

public:

    // 엔티티 생성
    Entity CreateEntity();
    // 엔티티 제거
    void DestroyEntity(Entity entity);
    
    // 특정 컴포넌트를 가진 모든 엔티티 가져오기
    template<typename T>
    std::vector<Entity> GetEntitiesWithComponent() const;

    // 여러 컴포넌트를 모두 가진 엔티티들 가져오기
    template<typename T1, typename T2, typename... Rest>
    std::vector<Entity> GetEntitiesWithComponents() const;

public:
    // 컴포넌트 추가
    template<typename T, typename... Args>
    T& AddComponent(Entity entity, Args&&... args);

    // 컴포넌트 제거
    template<typename T>
    void RemoveComponent(Entity entity);

    // 컴포넌트 가져오기
    template<typename T>
    T* GetComponent(Entity entity);
    template<typename T>
    const T* GetComponent(Entity entity) const;

    // 컴포넌트 존재 여부 확인
    template<typename T>
    bool HasComponent(Entity entity) const;

    // 컴포넌트 풀 직접 접근 (성능 최적화용)
    template<typename T>
    ComponentPool<T>& GetComponentPool();

    template<typename T>
    const ComponentPool<T>& GetComponentPool() const;

public:
    // 전체 정리
    void Clear();

    // 디버그 정보
    size_t GetEntityCount() const { return mEntities.size(); }
    size_t GetComponentPoolCount() const { return mComponentPools.size(); }

private:
    // Entity
    EntityID mNextEntityID;
    std::vector<Entity> mEntities;

    // 타입별 컴포넌트 풀 (type erasure 사용)
    std::unordered_map<ComponentTypeID, std::unique_ptr<BaseComponentPool>> mComponentPools;

    // System
    std::shared_ptr<SystemManager>		mSystemManager = std::make_shared< SystemManager>(this);

	void RemoveComponentFromPool(EntityID entityID, ComponentTypeID typeID);
};


template<typename T, typename... Args>
T& World::AddComponent(Entity entity, Args&&... args) {
    static_assert(std::is_base_of_v<Component<T>, T>, "T must inherit from Component<T>");

    ComponentTypeID typeID = T::GetTypeID();

    // 풀이 없으면 생성
    if (mComponentPools.find(typeID) == mComponentPools.end()) {
        mComponentPools[typeID] = std::make_unique<ComponentPool<T>>();
    }

    auto& pool = GetComponentPool<T>();
    T component(std::forward<Args>(args)...);
    pool.AddComponent(entity.GetID(), std::move(component));

    return *pool.GetComponent(entity.GetID());
}

template<typename T>
void World::RemoveComponent(Entity entity) {
    ComponentTypeID typeID = T::GetTypeID();
    RemoveComponentFromPool(entity.GetID(), typeID);
}

template<typename T>
T* World::GetComponent(Entity entity) {
    auto& pool = GetComponentPool<T>();
    return pool.GetComponent(entity.GetID());
}

template<typename T>
const T* World::GetComponent(Entity entity) const {
    ComponentTypeID typeID = T::GetTypeID();
    auto it = mComponentPools.find(typeID);
    if (it == mComponentPools.end()) return nullptr; // 안전하게 nullptr
    const auto* pool = static_cast<const ComponentPool<T>*>(it->second.get());
    return pool->GetComponent(entity.GetID());
}

template<typename T>
bool World::HasComponent(Entity entity) const {
    ComponentTypeID typeID = T::GetTypeID();
    auto it = mComponentPools.find(typeID);
    if (it == mComponentPools.end()) return false; // 풀이 없으면 false
    const auto* pool = static_cast<const ComponentPool<T>*>(it->second.get());
    return pool->HasComponent(entity.GetID());
}

template<typename T>
std::vector<Entity> World::GetEntitiesWithComponent() const {
    const auto& pool = GetComponentPool<T>();
    const auto& entityIDs = pool.GetEntities();


    std::vector<Entity> result;
    result.reserve(entityIDs.size());

    for (EntityID id : entityIDs) {
        result.emplace_back(id);
    }

    return result;  

}


template<typename T1, typename T2, typename... Rest>
std::vector<Entity>  World::GetEntitiesWithComponents() const {
    auto entities1 = GetEntitiesWithComponent<T1>();

    std::vector<Entity> result;
    result.reserve(entities1.size());

    for (const Entity& entity : entities1) {
        if (HasComponent<T2>(entity) && (HasComponent<Rest>(entity) && ...)) {
            result.push_back(entity);
        }
    }

    return result;
}

template<typename T>
ComponentPool<T>& World::GetComponentPool() {
    ComponentTypeID typeID = T::GetTypeID();
    auto it = mComponentPools.find(typeID);

    if (it == mComponentPools.end()) {
        mComponentPools[typeID] = std::make_unique<ComponentPool<T>>();
        it = mComponentPools.find(typeID);
    }

    return *static_cast<ComponentPool<T>*>(it->second.get());
}

template<typename T>
const ComponentPool<T>& World::GetComponentPool() const {
    ComponentTypeID typeID = T::GetTypeID();
    auto it = mComponentPools.find(typeID);

    assert(it != mComponentPools.end() && "Component pool not found");
    return *static_cast<const ComponentPool<T>*>(it->second.get());
}
