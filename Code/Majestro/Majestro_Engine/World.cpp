#include "pch.h"
#include "World.h"



Entity World::CreateEntity() {
    EntityID id = mNextEntityID++;
    mEntities.emplace_back(id);
    return Entity(id);
}

void World::DestroyEntity(Entity entity) {
    EntityID id = entity.GetID();
    if (id == NULL_ENTITY) return;

    // 모든 컴포넌트 풀에서 해당 엔티티의 컴포넌트 제거
    for (auto& pool : mComponentPools) {
        if (pool)
            pool->RemoveComponent(id);
    }

    // 엔티티 목록에서 제거
    mEntities.erase(
        std::remove_if(mEntities.begin(), mEntities.end(),
            [id](const Entity& e) { return e.GetID() == id; }),
        mEntities.end()
    );
}


void World::Clear() {
    mEntities.clear();
    mComponentPools.clear();
    mNextEntityID = 1;
    mSingletonEntity = Entity{};


    if (mNetIdMap)
        mNetIdMap->ClearScene(mSceneId);
}

void World::RemoveComponentFromPool(EntityID entityID, ComponentTypeID typeID)
{
    if (typeID < mComponentPools.size() && mComponentPools[typeID]) {
        mComponentPools[typeID]->RemoveComponent(entityID);
	}
}

void World::Shutdown()
{
	mSystemManager->Shutdown();
    // Clear();
}