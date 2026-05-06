#include "pch.h"
#include "EnemySystem.h"

#include "GameCore.h"
#include "ResourceManager.h"
#include "NavMeshLoader.h"

#include "TransformComponent.h"
#include "MovementComponent.h"

#include "BeatSystem.h"
#include "EnemyComponent.h"
#include "HealthComponent.h"
#include "GravityComponent.h"
#include "EventManager.h"
#include "GameEvents.h"
#include "GameTimer.h"



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

    auto systemManager = mWorld->GetSystemManager();
    auto* beatSystem = systemManager->GetSystem<BeatSystem>();
    const float Beat = beatSystem->mBpmSeconds;

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

    std::shared_ptr<EventManager> eventManager = mWorld->GetEventManager();
    shared_ptr<Navigation> navSystem = mWorld->GetNavSystem();
    const float now = GetServerTotalTimeSeconds();
    bool loggedNearbyThisFrame = false;

    int entityIndex = 0;
    for (auto& entity : mWorld->GetEntitiesWithComponent<EnemyMovementComponent>())
    {
        TransformComponent*    tf  = mWorld->GetComponent<TransformComponent>(entity);

        EnemyMovementComponent* mc = mWorld->GetComponent<EnemyMovementComponent>(entity);

        if (!tf || !mc) { ++entityIndex; continue; }

        const Vec3 myPos      = tf->mLocalPosition;
        const Vec3 playerPos  = PathFinder(myPos);
        const std::vector<Entity> nearbyEnemies = mWorld->GetPhysicsWorld()
            ? mWorld->GetPhysicsWorld()->FindNearbyEnemies(entity, NEARBY_ENEMY_RADIUS, MAX_NEARBY_ENEMIES)
            : std::vector<Entity>{};

        EnemyComponent* enemyComp = mWorld->GetComponent<EnemyComponent>(entity);
        HealthComponent* enemyHealthComp = mWorld->GetComponent<HealthComponent>(entity);
        if (enemyComp == nullptr) { ++entityIndex; continue; }
        mc->mMovingSpeed = enemyComp->mSpeed;


        if (enemyHealthComp && enemyHealthComp->mCurrentHp <= 0)
        {
            HaltByState(enemyComp, mc, EnemyAnimState::Dead);
            ++entityIndex;
            continue;
        }

        if (!loggedNearbyThisFrame)
        {
            std::cout << "[SpatialDebug] enemy=" << entity.GetID()
                << " nearby_count=" << nearbyEnemies.size();

            for (const Entity& nearbyEntity : nearbyEnemies)
            {
                const TransformComponent* nearbyTf = mWorld->GetComponent<TransformComponent>(nearbyEntity);
                if (!nearbyTf)
                    continue;

                Vec3 delta = nearbyTf->mLocalPosition - myPos;
                delta.y = 0.0f;
                const float distance = std::sqrt(delta.LengthSquared());

                std::cout << " | id=" << nearbyEntity.GetID()
                    << " dist=" << distance;
            }

            std::cout << std::endl;
            loggedNearbyThisFrame = true;
        }
       
        float nearestPlayerDistSq = (std::numeric_limits<float>::max)();
        for (auto& playerEntity : mWorld->GetEntitiesWithComponent<PlayerMovementComponent>())
        {
            TransformComponent* playerTf = transformPool.GetComponent(playerEntity.GetID());
            if (!playerTf)
                continue;

            const float distSq = Vec3::DistanceSquared(myPos, playerTf->mLocalPosition);
            nearestPlayerDistSq = (std::min)(nearestPlayerDistSq, distSq);
        }

        EnemyAnimState currentState = EnemyAnimState::Run;
        if (nearestPlayerDistSq <= enemyComp->AttackRangeSq)
            currentState = EnemyAnimState::Attack;

        if (currentState == EnemyAnimState::Attack && HandleAttackState(entity, enemyComp, mc, nearestPlayerDistSq, Beat, now, eventManager))
        {
            ++entityIndex;
            continue;
        }

        if (currentState != EnemyAnimState::Run)
        {
            HaltByState(enemyComp, mc, currentState);

            ++entityIndex;
            continue;
        }

        HandleRunState(entity, enemyComp, mc, myPos, playerPos, navSystem, dt, entityIndex);

        ++entityIndex;
    }
}

bool EnemySystem::HandleAttackState(
    const Entity& entity,
    EnemyComponent* enemyComp,
    EnemyMovementComponent* movementComp,
    float nearestPlayerDistSq,
    float beatSeconds,
    float nowSeconds,
    const std::shared_ptr<EventManager>& eventManager)
{
    if (!enemyComp || !movementComp)
        return false;

    if (nearestPlayerDistSq > enemyComp->AttackRangeSq)
    {
        enemyComp->mPendingAttackTime = -1.0f;
        return false;
    }

    Vec3 myPos = Vec3::Zero;
    Vec3 playerPos = Vec3::Zero;
    if (TransformComponent* tf = mWorld->GetComponent<TransformComponent>(entity))
    {
        myPos = tf->mLocalPosition;
        playerPos = PathFinder(myPos);
    }

    switch (enemyComp->mEnemyType)
    {
    case EnemyType::HornMan:
        movementComp->mMovingDirection = Vec3::Zero;
        movementComp->mPathCount = 0;
        movementComp->mPathIndex = 0;

        if (eventManager && enemyComp->mNextAttackTime <= nowSeconds)
        {
            eventManager->Enqueue<EvRangedAttackRequest>({ entity, SkillType::HornAttack });
            enemyComp->mNextAttackTime = nowSeconds + beatSeconds * enemyComp->mAttackCool;
            enemyComp->mAttackAnimEndTime = nowSeconds + enemyComp->mAttackAnimTime;
        }
        break;
    case EnemyType::Pianoman:
    {
        movementComp->mPathCount = 0;
        movementComp->mPathIndex = 0;
        movementComp->mMovingSpeed = enemyComp->mSpeed * 3.0f;

        Vec3 rushDir = playerPos - myPos;
        rushDir.y = 0.0f;
        if (rushDir.LengthSquared() > 1e-8f)
        {
            rushDir.Normalize();
            movementComp->mMovingDirection = rushDir;
        }
        else
        {
            movementComp->mMovingDirection = Vec3::Zero;
        }

        constexpr float kPianoMeleeRange = 160.0f;
        if (eventManager && enemyComp->mNextAttackTime <= nowSeconds && nearestPlayerDistSq <= kPianoMeleeRange * kPianoMeleeRange)
        {
            eventManager->Enqueue<EvMeleeAttackRequest>({ entity, SkillType::PianoAttack });
            enemyComp->mNextAttackTime = nowSeconds + beatSeconds * enemyComp->mAttackCool;
            enemyComp->mAttackAnimEndTime = nowSeconds + enemyComp->mAttackAnimTime;
        }
        break;
    }
    case EnemyType::Bongoman:
        movementComp->mMovingDirection = Vec3::Zero;
        movementComp->mPathCount = 0;
        movementComp->mPathIndex = 0;

        if (enemyComp->mPendingAttackTime < nowSeconds)
            enemyComp->mPendingAttackTime = nowSeconds + beatSeconds * 4.0f;

        if (eventManager && enemyComp->mNextAttackTime <= nowSeconds && nowSeconds >= enemyComp->mPendingAttackTime)
        {
            eventManager->Enqueue<EvMeleeAttackRequest>({ entity, SkillType::BongoAttack });
            enemyComp->mNextAttackTime = nowSeconds + beatSeconds * enemyComp->mAttackCool;
            enemyComp->mAttackAnimEndTime = nowSeconds + enemyComp->mAttackAnimTime;
            enemyComp->mPendingAttackTime = -1.0f;
        }
        break;
    default:
        return false;
    }

    if (nowSeconds <= enemyComp->mAttackAnimEndTime)
        enemyComp->mAnimState = static_cast<uint8>(EnemyAnimState::Attack);
    else
        enemyComp->mAnimState = static_cast<uint8>(EnemyAnimState::Run);

    return true;
}

void EnemySystem::HandleRunState(
    const Entity& entity,
    EnemyComponent* enemyComp,
    EnemyMovementComponent* movementComp,
    const Vec3& myPos,
    const Vec3& playerPos,
    const std::shared_ptr<Navigation>& navSystem,
    float dt,
    int entityIndex)
{
    if (!enemyComp || !movementComp)
        return;

    // ---- 재탐색 판단 ----
    movementComp->mPathTimer -= dt;

    const bool targetMoved = Vec3::DistanceSquared(playerPos, movementComp->mTarget) > RETARGET_THRESHOLD_SQ;
    const bool needRepath = (movementComp->mPathTimer <= 0.f) || targetMoved || (movementComp->mPathCount == 0);

    if (needRepath)
    {
        movementComp->mTarget = playerPos;

        bool ok = false;
        if (navSystem && navSystem->IsInitialized())
        {
            ok = navSystem->FindPath(myPos, playerPos, movementComp->mPath, movementComp->mPathCount, ENEMY_MAX_WAYPOINTS);
        }

        if (!ok)
        {
            // NavMesh 탐색 실패 시 직선 방향 (직진)
            movementComp->mPathCount = 1;
            movementComp->mPath[0] = playerPos;
        }

        movementComp->mPathIndex = 0;

        // 재탐색 쿨타임 — 엔티티 인덱스로 분산시켜 동일 프레임 스파이크 방지
        const float stagger = movementComp->mPathInterval / 20.f; // 최대 20개 분산
        movementComp->mPathTimer = movementComp->mPathInterval + (entityIndex % 20) * stagger;
    }

    // ---- 경로 추적 ----
    if (movementComp->mPathCount > 0 && movementComp->mPathIndex < movementComp->mPathCount)
    {
        // 현재 웨이포인트에 도달했으면 다음으로
        while (movementComp->mPathIndex < movementComp->mPathCount - 1)
        {
            Vec3 toWp = movementComp->mPath[movementComp->mPathIndex] - myPos;
            if (toWp.LengthSquared() < ARRIVE_THRESHOLD_SQ)
                ++movementComp->mPathIndex;
            else
                break;
        }

        Vec3 dir = movementComp->mPath[movementComp->mPathIndex] - myPos;
        dir.y = 0.f;
        if (dir.LengthSquared() > 1e-8f)
        {
            dir.Normalize();
            movementComp->mMovingDirection = dir;
            enemyComp->mAnimState = static_cast<uint8>(EnemyAnimState::Run);
        }
        else
        {
            movementComp->mMovingDirection = Vec3::Zero;
            enemyComp->mAnimState = static_cast<uint8>(EnemyAnimState::Run);
        }
    }
}

void EnemySystem::HaltByState(EnemyComponent* enemyComp, EnemyMovementComponent* movementComp, EnemyAnimState state)
{
    if (!enemyComp || !movementComp)
        return;

    enemyComp->mAnimState = static_cast<uint8>(state);
    movementComp->mMovingDirection = Vec3::Zero;
    movementComp->mPathCount = 0;
    movementComp->mPathIndex = 0;
}

Vec3 EnemySystem::PathFinder(const Vec3& from)
{
    auto FlatDistSq = [](const Vec3& a, const Vec3& b) -> float
        {
            const float dx = a.x - b.x;
            const float dz = a.z - b.z;
            return dx * dx + dz * dz;
        };

    Vec3  nearest = mPlayerPositions[0];
    float minDistSq = FlatDistSq(from, nearest);
    for (size_t i = 1; i < mPlayerPositions.size(); ++i)
    {
        const float d = FlatDistSq(from, mPlayerPositions[i]);
        if (d < minDistSq) { minDistSq = d; nearest = mPlayerPositions[i]; }
    }
    nearest.y = from.y;
    return nearest;
}
