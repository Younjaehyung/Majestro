#include "pch.h"
#include "World.h"
#include "NetEntityComponent.h"


Entity World::CreateEntity() {
    EntityID id = mNextEntityID++;
    mEntities.emplace_back(id);
    return Entity(id);
}

void World::DestroyEntity(Entity entity) {
    EntityID id = entity.GetID();
    if (id == NULL_ENTITY) return;

    UnregisterActiveBullet(entity);

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
    mActiveBulletEntityIds.clear();
}

uint32 World::GetSessionIDByEntity(Entity entity)
{
	NetEntityComponent* comp = GetComponent<NetEntityComponent>(entity);
    if(comp!=nullptr)
        return comp->mSessionId;

    return 0;
}

void World::RegisterActiveBullet(Entity bulletEntity)
{
    if (!bulletEntity.IsValid())
        return;

    const EntityID bulletId = bulletEntity.GetID();
    for (EntityID activeBulletId : mActiveBulletEntityIds)
    {
        if (activeBulletId == bulletId)
            return;
    }

    mActiveBulletEntityIds.push_back(bulletId);
}

void World::UnregisterActiveBullet(Entity bulletEntity)
{
    if (!bulletEntity.IsValid())
        return;

    const EntityID bulletId = bulletEntity.GetID();
    for (size_t i = 0; i < mActiveBulletEntityIds.size(); ++i)
    {
        if (mActiveBulletEntityIds[i] != bulletId)
            continue;

        mActiveBulletEntityIds[i] = mActiveBulletEntityIds.back();
        mActiveBulletEntityIds.pop_back();
        return;
    }
}

void World::RemoveComponentFromPool(EntityID entityID, ComponentTypeID typeID)
{
    auto it = mComponentPools.find(typeID);
    if (it != mComponentPools.end()) {
        it->second->RemoveComponent(entityID);
	}
}
