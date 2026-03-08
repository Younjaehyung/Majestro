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
        TransformComponent*  tf = mWorld->GetComponent<TransformComponent>(entity);
        if (!tf) continue;

        // XZ 이동량이 거의 없으면 검증 불필요
        const float moveXSq = tf->mMovingVector.x * tf->mMovingVector.x
                            + tf->mMovingVector.z * tf->mMovingVector.z;
        if (moveXSq < MIN_MOVE_SQ) continue;

        // 이동 전 XZ 위치 복원 (mMovingVector = 이번 프레임 이동 델타)
        Vec3 prevPos = tf->mLocalPosition;
        prevPos.x -= tf->mMovingVector.x;
        prevPos.z -= tf->mMovingVector.z;
        // Y: 이동 전후 동일 (Y 검증은 중력 시스템에 위임)
        prevPos.y  = tf->mLocalPosition.y;

        // 엔진(cm) : NavMesh(m), Y는 0으로 평탄화해 지표면 기준 XZ 검증
        Vec3 prevM = prevPos * 0.01f;  prevM.y = 0.f;
        Vec3 newM  = tf->mLocalPosition * 0.01f; newM.y = 0.f;

        const float t = nav->Raycast(prevM, newM);

        if (t < 1.0f)
        {
            // 이동 가능 거리만큼 클램프
            tf->mLocalPosition.x = prevPos.x + tf->mMovingVector.x * t;
            tf->mLocalPosition.z = prevPos.z + tf->mMovingVector.z * t;
            tf->mMovingVector.x *= t;
            tf->mMovingVector.z *= t;
        }
    }
}
