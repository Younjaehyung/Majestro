#include "pch.h"
#include "PlayerNavValidationSystem.h"
#include "TransformComponent.h"
#include "MovementComponent.h"
#include "GravityComponent.h"
#include "NavMeshLoader.h"


PlayerNavValidationSystem::PlayerNavValidationSystem(World* world) : System(world)
{
}

void PlayerNavValidationSystem::Update(float dt)
{
    if (!mWorld->HasComponentPool<PlayerMovementComponent>()) return;
    if (!mWorld->HasComponentPool<TransformComponent>())      return;

    shared_ptr<Navigation>& nav = mWorld->GetNavSystem();
    if (!nav || !nav->IsInitialized()) return;

    for (auto& entity : mWorld->GetEntitiesWithComponent<PlayerMovementComponent>())
    {
        TransformComponent*      tf  = mWorld->GetComponent<TransformComponent>(entity);
        PlayerMovementComponent* pmc = mWorld->GetComponent<PlayerMovementComponent>(entity);
        if (!tf || !pmc) continue;

        // Jolt 위치를 NavMesh 표면에 투영(가장 가까운 폴리곤 위 점).
        // 성공 시에만 갱신
        if (nav->IsPointOnNavMesh(tf->mLocalPosition, kProjectRadius))
        {
            pmc->mNavPosition      = nav->GetNearestPointOnNavMesh(tf->mLocalPosition, kProjectRadius);
            pmc->mNavPositionValid = true;
        }
    }
}
