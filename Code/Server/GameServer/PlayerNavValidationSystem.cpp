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
    // return;
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
        prevPos.y = tf->mLocalPosition.y;

        // NavMesh 표면을 따라 이동 — 벽이 있으면 자동으로 막히는 위치로 클램프
        Vec3 resultPos;
        if (nav->MoveAlongSurface(prevPos, tf->mLocalPosition, resultPos))
        {
            tf->mLocalPosition.x = resultPos.x;
			//tf->mLocalPosition.y = resultPos.y; // Y는 NavMesh 높이로 보정 (낙하/점프는 중력 시스템에 위임)
            tf->mLocalPosition.z = resultPos.z;
            tf->mMovingVector.x  = resultPos.x - prevPos.x;
            tf->mMovingVector.y = 0;
            tf->mMovingVector.z  = resultPos.z - prevPos.z;

            GravityComponent* gravityComp = mWorld->GetComponent<GravityComponent>(entity);
            if (gravityComp) {
                gravityComp->mGround = resultPos.y;
                //gravityComp->mHight = resultPos.y; // NavMesh 높이는 중력 단계에서 최종 반영
            }
        }
        // MoveAlongSurface가 false(NavMesh 밖)이면 검증 스킵  이동 그대로
    }
}
