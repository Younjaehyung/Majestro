#include "pch.h"
#include "EnemySystem.h"

#include "GameCore.h"
#include "ResourceManager.h"
#include "NavMeshLoader.h"

#include "TransformComponent.h"
#include "MovementComponent.h"



EnemySystem::EnemySystem(World* world) : System(world)
{
}

void EnemySystem::Initialize()
{

    mNavMesh = RESOURCEMANAGER.Get<NavMesh>(L"NavMesh");
    if (mNavMesh == nullptr)
    {
        wstring navMeshPath = L"../Resources/NavMesh/navmesh.bin";
        mNavMesh = make_shared<NavMesh>();
        mNavMesh->Load(ws2s(navMeshPath));
        RESOURCEMANAGER.Add<NavMesh>(L"NavMesh", mNavMesh);
    }
    if (mNavMesh && mNavMesh->mDtNavMesh)
        mWorld->GetNavSystem()->Initialize(mNavMesh);

	


}


void EnemySystem::Update(float dt)
{

    if (!mWorld->HasComponentPool<EnemyMovementComponent>()) return;
    if (!mWorld->HasComponentPool<TransformComponent>())     return;

    auto& transformPool = mWorld->GetComponentPool<TransformComponent>();

    // 프레임 별 플레이어 위치 목록을 미리 수집
    mPlayerPositions.clear();
    if (mWorld->HasComponentPool<PlayerMovementComponent>())
    {
        for (auto& playerEntity : mWorld->GetEntitiesWithComponent<PlayerMovementComponent>())
        {
            TransformComponent* tf = transformPool.GetComponent(playerEntity.GetID());
            if (tf) mPlayerPositions.push_back(tf->mLocalPosition);
        }
    }

    // 플레이어 없으면 리턴
    if (mPlayerPositions.empty()) return;

    int entityIndex = 0;
    for (auto& entity : mWorld->GetEntitiesWithComponent<EnemyMovementComponent>())
    {
        TransformComponent*    tf  = mWorld->GetComponent<TransformComponent>(entity);

        EnemyMovementComponent* mc = mWorld->GetComponent<EnemyMovementComponent>(entity);

        if (!tf || !mc) { ++entityIndex; continue; }

        const Vec3 myPos      = tf->mLocalPosition;
        const Vec3 playerPos  = PathFinder(myPos);

        // ---- 재탐색 판단 ----
        mc->mPathTimer -= dt;

        const bool targetMoved = Vec3::DistanceSquared(playerPos, mc->mTarget) > RETARGET_THRESHOLD_SQ;
        const bool needRepath  = (mc->mPathTimer <= 0.f) || targetMoved || (mc->mPathCount == 0);

        if (needRepath)
        {
            mc->mTarget = playerPos;

            bool ok = false;
            shared_ptr<Navigation> nav = mWorld->GetNavSystem();
            if (nav && nav->IsInitialized())
            {
                ok = nav->FindPath(myPos, playerPos, mc->mPath, mc->mPathCount, ENEMY_MAX_WAYPOINTS);
            }

            if (!ok)
            {
                // NavMesh 탐색 실패 시 직선 방향 (직진)
                mc->mPathCount   = 1;
                mc->mPath[0]     = playerPos;
				//std::cout << "Pathfinding failed for entity " << entity.GetID() << ". Moving directly towards target." << std::endl;
            }
            else {
                // 탐색 성공 시 디버그 로그
				//std::cout << "Pathfinding succeeded for entity " << entity.GetID() << ". Waypoints: " << mc->mPathCount << std::endl;
            }

            mc->mPathIndex = 0;

            // 재탐색 쿨타임 — 엔티티 인덱스로 분산시켜 동일 프레임 스파이크 방지
            const float stagger  = mc->mPathInterval / 20.f; // 최대 20개 분산
            mc->mPathTimer = mc->mPathInterval + (entityIndex % 20) * stagger;
        }

        // ---- 경로 추적 ----
        if (mc->mPathCount > 0 && mc->mPathIndex < mc->mPathCount)
        {
            // 현재 웨이포인트에 도달했으면 다음으로
            while (mc->mPathIndex < mc->mPathCount - 1)
            {
                Vec3 toWp = mc->mPath[mc->mPathIndex] - myPos;
                //toWp.y    = 0.f;
                if (toWp.LengthSquared() < ARRIVE_THRESHOLD_SQ)
                    ++mc->mPathIndex;
                else
                    break;
            }

            Vec3 dir = mc->mPath[mc->mPathIndex] - myPos;
            //dir.y    = 0.f;
            dir.Normalize();
            mc->mMovingDirection = dir;
        }

        ++entityIndex;
    }
}

Vec3 EnemySystem::PathFinder(const Vec3& from)
{

    Vec3  nearest = mPlayerPositions[0];
    float minDistSq = Vec3::DistanceSquared(from, nearest);
    for (size_t i = 1; i < mPlayerPositions.size(); ++i)
    {
        float d = Vec3::DistanceSquared(from, mPlayerPositions[i]);
        if (d < minDistSq) { minDistSq = d; nearest = mPlayerPositions[i]; }
    }
    return nearest;
}
