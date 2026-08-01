#pragma once

#include "Entity.h"
#include "Component.h"
#include "ComponentPool.h"
#include "SystemManager.h"
#include "EventManager.h"
#include "NetIdMap.h"
#include "PhysicsWorld.h"
#include <tuple>



template<typename... Components>
class EntityView;
class World {
public:
    World() : mNextEntityID(1) {
        mPhysicsWorld = std::make_shared<PhysicsWorld>();
        mEventManager = make_shared<EventManager>();
    }
    void Initialize() {
        mSystemManager = std::make_shared<SystemManager>(this);
		mSingletonEntity = CreateEntity();
    }
    void Update(float deltaTime) { mSystemManager->Update(deltaTime); }
    void Render() { mSystemManager->Render(); }
    void Shutdown();
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

    // 컴포넌트 기반 뷰
    template<typename... Components>
    EntityView<Components...> View() const;

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

    template<typename T>
    bool HasComponentPool() const;

    // 컴포넌트 풀 직접 접근 (성능 최적화용)
    template<typename T>
    ComponentPool<T>& GetComponentPool();

    template<typename T>
    const ComponentPool<T>& GetComponentPool() const;

public:
    // 싱글톤 컴포넌트
    Entity GetSingletonEntity() const { return mSingletonEntity; }

    template<typename T, typename... Args>
    T& AddSingleton(Args&&... args) { return AddComponent<T>(mSingletonEntity, std::forward<Args>(args)...); }

    template<typename T>
    T* GetSingleton() { return GetComponent<T>(mSingletonEntity); }

    template<typename T>
    const T* GetSingleton() const { return GetComponent<T>(mSingletonEntity); }

    template<typename T>
    void RemoveSingleton() { RemoveComponent<T>(mSingletonEntity); }

public:
    // 전체 정리
    void Clear();

    // 디버그 정보
    size_t GetEntityCount() const { return mEntities.size(); }
    size_t GetComponentPoolCount() const {
        size_t count = 0;
        for (const auto& pool : mComponentPools) {
            if (pool) ++count;
        }
        return count;
    }

    // Network Entity ID
    shared_ptr<NetIdMap>& GetNetIdMap() { return mNetIdMap; }
    void SetSceneId(SceneId sceneId) { mSceneId = sceneId; }
    SceneId GetSceneId() const { return mSceneId; }

    // System Manager
    std::shared_ptr<SystemManager>& GetSystemManager() { return mSystemManager; }
    void SetSystemManager(std::shared_ptr<SystemManager> systemManager) { mSystemManager = systemManager; }

	// Physics World
    std::shared_ptr<PhysicsWorld>& GetPhysicsWorld() { return mPhysicsWorld; }
    
	// Event Manager
	std::shared_ptr<EventManager>& GetEventManager() { return mEventManager; }


public:
    void NetIdBinding(uint64 netID, Entity entity) { mNetIdMap->Bind(mSceneId, netID, entity); }
    void NetIdUnbinding(uint64 netID) { mNetIdMap->Unbind(mSceneId, netID); }
    Entity GetEntityByNetId(uint64 netID) const { return mNetIdMap->GetOrInvalid(mSceneId, netID); }
private:
    // Entity
    EntityID mNextEntityID;
    std::vector<Entity> mEntities;

	// Network Entity ID 관리
	shared_ptr<NetIdMap> mNetIdMap = make_shared<NetIdMap>();
    SceneId mSceneId = SceneId::Lobby;

    // 타입별 컴포넌트 풀 (type erasure 사용)
    std::vector<std::unique_ptr<BaseComponentPool>> mComponentPools;

    // System
    std::shared_ptr<SystemManager>		mSystemManager;
    std::shared_ptr<PhysicsWorld>       mPhysicsWorld;
    std::shared_ptr<EventManager>       mEventManager;


	Entity mSingletonEntity;	// 싱글톤 컴포넌트 전용 엔티티

	void RemoveComponentFromPool(EntityID entityID, ComponentTypeID typeID);

    // 풀 조회
    template<typename T>
    ComponentPool<T>* FindComponentPool() {
        const ComponentTypeID typeID = T::GetTypeID();
        if (typeID >= mComponentPools.size()) return nullptr;
        return static_cast<ComponentPool<T>*>(mComponentPools[typeID].get());
    }

    template<typename T>
    const ComponentPool<T>* FindComponentPool() const {
        const ComponentTypeID typeID = T::GetTypeID();
        if (typeID >= mComponentPools.size()) return nullptr;
        return static_cast<const ComponentPool<T>*>(mComponentPools[typeID].get());
    }
};

template<typename T>
bool World::HasComponentPool() const
{
    return FindComponentPool<T>() != nullptr;
}

template<typename T, typename... Args>
T& World::AddComponent(Entity entity, Args&&... args) {
    static_assert(std::is_base_of_v<Component<T>, T>, "T must inherit from Component<T>");

    // GetComponentPool 이 없는 풀을 만들어 준다.
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
    auto* pool = FindComponentPool<T>();
    if (pool == nullptr) return nullptr;
    return pool->GetComponent(entity.GetID());
}

template<typename T>
const T* World::GetComponent(Entity entity) const {
    const auto* pool = FindComponentPool<T>();
    if (pool == nullptr) return nullptr; // 안전하게 nullptr
    return pool->GetComponent(entity.GetID());
}

template<typename T>
bool World::HasComponent(Entity entity) const {
    const auto* pool = FindComponentPool<T>();
    if (pool == nullptr) return false; // 풀이 없으면 false
    return pool->HasComponent(entity.GetID());
}

template<typename T>
std::vector<Entity> World::GetEntitiesWithComponent() const {
    // 풀이 없으면 빈 목록. 가드가 없으면 아래 const GetComponentPool 의 assert 에 걸린다.
    if (!HasComponentPool<T>()) {
        return {};
    }

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
    const ComponentTypeID typeID = T::GetTypeID();

    if (typeID >= mComponentPools.size())
        mComponentPools.resize(typeID + 1);

    if (mComponentPools[typeID] == nullptr)
        mComponentPools[typeID] = std::make_unique<ComponentPool<T>>();

    return *static_cast<ComponentPool<T>*>(mComponentPools[typeID].get());
}

template<typename T>
const ComponentPool<T>& World::GetComponentPool() const {
    const ComponentTypeID typeID = T::GetTypeID();

    assert(typeID < mComponentPools.size() && mComponentPools[typeID] != nullptr && "Component pool not found");

    return *static_cast<const ComponentPool<T>*>(mComponentPools[typeID].get());
}


template<typename... Components>
EntityView<Components...> World::View() const {
    using FirstComponent = std::tuple_element_t<0, std::tuple<Components...>>;
    if (!HasComponentPool<FirstComponent>()) {
        return EntityView<Components...>(this, nullptr);
    }
    const auto& pool = GetComponentPool<FirstComponent>();
    return EntityView<Components...>(this, &pool.GetEntities());
}


template<typename... Components>
class EntityView {
public:
    class iterator {
    public:
        iterator(const World* world, const std::vector<EntityID>* entities, size_t index, bool skipAdvance)
            : mWorld(world)
            , mEntities(entities)
            , mIndex(index)
            , mEnd(entities ? entities->size() : 0) {
            if (!skipAdvance) {
                AdvanceToValid();
            }
        }

        Entity operator*() const { return Entity((*mEntities)[mIndex]); }

        iterator& operator++() {
            ++mIndex;
            AdvanceToValid();
            return *this;
        }

        bool operator==(const iterator& other) const {
            return mEntities == other.mEntities && mIndex == other.mIndex;
        }

        bool operator!=(const iterator& other) const { return !(*this == other); }

    private:
        void AdvanceToValid() {
            if (!mEntities) {
                mIndex = 0;
                mEnd = 0;
                return;
            }

            while (mIndex < mEnd) {
                Entity entity((*mEntities)[mIndex]);
                if (Matches(entity)) {
                    break;
                }
                ++mIndex;
            }
        }

        bool Matches(Entity entity) const {
            return (mWorld->HasComponent<Components>(entity) && ...);
        }

        const World* mWorld = nullptr;
        const std::vector<EntityID>* mEntities = nullptr;
        size_t mIndex = 0;
        size_t mEnd = 0;
    };

    EntityView(const World* world, const std::vector<EntityID>* entities)
        : mWorld(world)
        , mEntities(entities) {
    }

    iterator begin() const { return iterator(mWorld, mEntities, 0, false); }
    iterator end() const { return iterator(mWorld, mEntities, mEntities ? mEntities->size() : 0, true); }

private:
    const World* mWorld = nullptr;
    const std::vector<EntityID>* mEntities = nullptr;
};