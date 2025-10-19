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
    for (auto& [typeID, pool] : mComponentPools) {
        RemoveComponentFromPool(id, typeID);
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
}

void World::RemoveComponentFromPool(EntityID entityID, ComponentTypeID typeID)
{
    auto it = mComponentPools.find(typeID);
    if (it != mComponentPools.end()) {
        it->second->RemoveComponent(entityID);
	}
}
