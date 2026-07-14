#include "pch.h"
#include "DecalSystem.h"

#include "World.h"
#include "DecalComponent.h"
#include "TransformComponent.h"


void DecalSystem::Update(float deltaTime)
{
    if (mWorld == nullptr)
        return;
    if (mWorld->HasComponentPool<DecalComponent>() == false)
        return;

    std::vector<Entity> expired;

    auto entities = mWorld->GetEntitiesWithComponent<DecalComponent>();
    for (Entity e : entities)
    {
        DecalComponent* decal = mWorld->GetComponent<DecalComponent>(e);
        if (decal == nullptr)
            continue;

        if (decal->FollowTarget.IsValid())
        {
            if (auto* t = mWorld->GetComponent<TransformComponent>(decal->FollowTarget)) 
            {   // 추적
                decal->Center = t->GetWorldPosition() + decal->FollowOffset;
            }
            else if (decal->DestroyWithTarget)
            {   // 정리
                expired.push_back(e);
                continue;
            }
        }

        decal->Elapsed += deltaTime;

        if (decal->Lifetime >= 0.0f && decal->Elapsed >= decal->Lifetime)
            expired.push_back(e);
    }

    for (Entity e : expired)
        mWorld->DestroyEntity(e);
}
