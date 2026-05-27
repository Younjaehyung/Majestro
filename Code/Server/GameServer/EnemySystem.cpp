#include "pch.h"
#include "EnemySystem.h"

#include "GameCore.h"
#include "ResourceManager.h"
#include "NavMeshLoader.h"

#include "TransformComponent.h"
#include "MovementComponent.h"

#include "BeatSystem.h"
#include "EnemyComponent.h"
#include "ArmorComponent.h"
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

    UpdateOnnxToggle();

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
          //  std::cout << "[SpatialDebug] enemy=" << entity.GetID() << " nearby_count=" << nearbyEnemies.size();

            for (const Entity& nearbyEntity : nearbyEnemies)
            {
                const TransformComponent* nearbyTf = mWorld->GetComponent<TransformComponent>(nearbyEntity);
                if (!nearbyTf)
                    continue;

                Vec3 delta = nearbyTf->mLocalPosition - myPos;
                delta.y = 0.0f;
                const float distance = std::sqrt(delta.LengthSquared());

               // std::cout << " | id=" << nearbyEntity.GetID()<< " dist=" << distance;
            }

           // std::cout << std::endl;
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
        const bool bongomanCommittedAttack =
            enemyComp->mEnemyType == EnemyType::Bongoman &&
            enemyComp->mPendingAttackTime >= 0.0f;
        if (nearestPlayerDistSq <= enemyComp->AttackRangeSq || bongomanCommittedAttack)
            currentState = EnemyAnimState::Attack;

        ArmorComponent* armorComp = mWorld->GetComponent<ArmorComponent>(entity);
        if (currentState == EnemyAnimState::Run &&
            enemyComp->mEnemyType == EnemyType::Bongoman &&
            armorComp != nullptr &&
            armorComp->mCurrentArmor <= 0 &&
            now >= enemyComp->mNextShildTime)
        {
            armorComp->mCurrentArmor = (std::min)(armorComp->mMaxArmor, 50);
            enemyComp->mShieldAnimEndTime = now + enemyComp->mShieldAnimTime;
            enemyComp->mNextShildTime = now + Beat * 4.0f;
            if (eventManager)
            {
                eventManager->Enqueue<EvArmorChanged>({ entity, armorComp->mCurrentArmor, armorComp->mMaxArmor });
                eventManager->Enqueue<EvEffectSpawn>({
                    static_cast<uint8>(SkillType::BongoShild),
                    myPos.x,
                    myPos.y,
                    myPos.z,
                    EffectSpawnReason::Fire,
                    0.0f,
                    0.0f,
                    0.0f
                });
            }

            currentState = EnemyAnimState::Shield;
        }
        else if (currentState == EnemyAnimState::Run &&
            enemyComp->mEnemyType == EnemyType::Bongoman &&
            enemyComp->mShieldAnimEndTime > now)
        {
            currentState = EnemyAnimState::Shield;
        }

        if (enemyComp->mEnemyType == EnemyType::Pianoman &&
            (currentState != EnemyAnimState::Attack || enemyComp->mNextAttackTime > now))
            enemyComp->mPianoRushVfxPlayed = false;

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

    const bool bongomanCommittedAttack =
        enemyComp->mEnemyType == EnemyType::Bongoman &&
        enemyComp->mPendingAttackTime >= 0.0f;
    if (nearestPlayerDistSq > enemyComp->AttackRangeSq && !bongomanCommittedAttack)
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
        constexpr float kPianoMeleeRange = 160.0f;

        movementComp->mPathCount = 0;
        movementComp->mPathIndex = 0;
        movementComp->mMovingSpeed = enemyComp->mSpeed * 1.5f;

        const bool pianoAttackOnCooldown = enemyComp->mNextAttackTime > nowSeconds;
        if (pianoAttackOnCooldown)
        {
            movementComp->mMovingDirection = Vec3::Zero;
        }
        else
        {
            Vec3 rushDir = playerPos - myPos;
            rushDir.y = 0.0f;

            const bool canTriggerRushVfx = nearestPlayerDistSq > kPianoMeleeRange * kPianoMeleeRange;
            if (rushDir.LengthSquared() > 1e-8f)
            {
                rushDir.Normalize();

                if (eventManager && !enemyComp->mPianoRushVfxPlayed && canTriggerRushVfx)
                {
                    const float rushYawDeg = DirectX::XMConvertToDegrees(std::atan2(rushDir.x, rushDir.z));
                    eventManager->Enqueue<EvEffectSpawn>({
                        static_cast<uint8>(SkillType::PianoAttack),
                        myPos.x,
                        myPos.y,
                        myPos.z,
                        EffectSpawnReason::Fire,
                        0.0f,
                        rushYawDeg,
                        0.0f
                    });
                    enemyComp->mPianoRushVfxPlayed = true;
                }

                movementComp->mMovingDirection = rushDir;
            }
            else
            {
                movementComp->mMovingDirection = Vec3::Zero;
            }
        }

        if (eventManager && enemyComp->mNextAttackTime <= nowSeconds && nearestPlayerDistSq <= kPianoMeleeRange * kPianoMeleeRange)
        {
            Vec3 attackDir = playerPos - myPos;
            attackDir.y = 0.0f;
            float attackYawDeg = 0.0f;
            if (attackDir.LengthSquared() > 1e-8f)
            {
                attackDir.Normalize();
                attackYawDeg = DirectX::XMConvertToDegrees(std::atan2(attackDir.x, attackDir.z));
            }

            eventManager->Enqueue<EvEffectSpawn>({
                static_cast<uint8>(SkillType::PianoAttack),
                myPos.x,
                myPos.y,
                myPos.z,
                EffectSpawnReason::LifetimeExpired,
                0.0f,
                attackYawDeg,
                0.0f
            });
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

        if (enemyComp->mPendingAttackTime < 0.0f)
            enemyComp->mPendingAttackTime = nowSeconds + beatSeconds * 4.0f;

        if (eventManager && enemyComp->mNextAttackTime <= nowSeconds && nowSeconds >= enemyComp->mPendingAttackTime)
        {
            eventManager->Enqueue<EvMeleeAttackRequest>({ entity, SkillType::BongoAttack });
            enemyComp->mNextAttackTime = nowSeconds + beatSeconds * enemyComp->mAttackCool;
            enemyComp->mAttackAnimEndTime = nowSeconds + enemyComp->mAttackAnimTime;
            enemyComp->mPendingAttackTime = -1.0f;
            std::cout << "[Bongo] attack queued enemy=" << entity.GetID()
                << " now=" << nowSeconds
                << " next=" << enemyComp->mNextAttackTime
                << " pending=" << enemyComp->mPendingAttackTime
                << std::endl;
        }
        break;
    default:
        return false;
    }

    if (enemyComp->mEnemyType == EnemyType::Pianoman)
    {
        const bool pianoAttackOnCooldown = enemyComp->mNextAttackTime > nowSeconds;
        enemyComp->mAnimState = static_cast<uint8>(pianoAttackOnCooldown ? EnemyAnimState::Run : EnemyAnimState::Attack);
    }
    else if (nowSeconds <= enemyComp->mAttackAnimEndTime)
    {
        enemyComp->mAnimState = static_cast<uint8>(EnemyAnimState::Attack);
    }
    else
    {
        enemyComp->mAnimState = static_cast<uint8>(EnemyAnimState::Run);
    }

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

    Vec3 desiredTarget = playerPos;
    if (mUseOnnxBaseMove)
    {
        Vec3 onnxTarget = playerPos;
        if (TryComputeOnnxBaseMoveTarget(entity, myPos, playerPos, onnxTarget))
            desiredTarget = onnxTarget;
    }

    // ---- 재탐색 판단 ----
    movementComp->mPathTimer -= dt;

    const bool targetMoved = Vec3::DistanceSquared(desiredTarget, movementComp->mTarget) > RETARGET_THRESHOLD_SQ;
    const bool needRepath = (movementComp->mPathTimer <= 0.f) || targetMoved || (movementComp->mPathCount == 0);

    if (needRepath)
    {
        movementComp->mTarget = desiredTarget;

        bool ok = false;
        if (navSystem && navSystem->IsInitialized())
        {
            ok = navSystem->FindPath(myPos, desiredTarget, movementComp->mPath, movementComp->mPathCount, ENEMY_MAX_WAYPOINTS);
        }

        if (!ok)
        {
            // NavMesh 탐색 실패 시 직선 방향 (직진)
            movementComp->mPathCount = 1;
            movementComp->mPath[0] = desiredTarget;
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

void EnemySystem::UpdateOnnxToggle()
{
    const bool pressed = (GetAsyncKeyState(VK_F6) & 0x8000) != 0;
    if (pressed && !mOnnxToggleKeyHeld)
    {
        mUseOnnxBaseMove = !mUseOnnxBaseMove;
        std::cout << "[EnemySystem] ONNX base_move " << (mUseOnnxBaseMove ? "enabled" : "disabled") << std::endl;
    }

    mOnnxToggleKeyHeld = pressed;
}

bool EnemySystem::TryComputeOnnxBaseMoveTarget(
    const Entity& entity,
    const Vec3& myPos,
    const Vec3& playerPos,
    Vec3& outTarget) const
{
    if (!AIMANAGER.HasModel(L"base_move"))
        return false;

    TransformComponent* myTransform = mWorld->GetComponent<TransformComponent>(entity);
    EnemyMovementComponent* myMovement = mWorld->GetComponent<EnemyMovementComponent>(entity);
    if (!myTransform || !myMovement)
        return false;

    AIManager::InputArray input{};
    const Vec3 center3(0.0f, 0.0f, 0.0f);
    const float scale = (std::max)(ONNX_MAP_RANGE, 1.0f);

    const Vec3 goalPos3(playerPos.x, playerPos.y, playerPos.z);
    const Vec3 delta3 = goalPos3 - myPos;

    const Vec3 agentNorm = (myPos - center3) / scale;
    const Vec3 goalNorm = (goalPos3 - center3) / scale;
    const Vec3 deltaNorm = delta3 / scale;
    const Vec2 velNorm(
        myMovement->mMovingDirection.x * (myMovement->mMovingSpeed / 120.0f),
        myMovement->mMovingDirection.z * (myMovement->mMovingSpeed / 120.0f));

    input[0] = agentNorm.x;
    input[1] = agentNorm.y;
    input[2] = agentNorm.z;
    input[3] = goalNorm.x;
    input[4] = goalNorm.y;
    input[5] = goalNorm.z;
    input[6] = deltaNorm.x;
    input[7] = deltaNorm.y;
    input[8] = deltaNorm.z;
    input[9] = velNorm.x;
    input[10] = velNorm.y;

    struct NearbyEnemyObs
    {
        float relX = 0.0f;
        float relZ = 0.0f;
        float dist = 0.0f;
    };

    std::vector<NearbyEnemyObs> nearby;
    nearby.reserve(MAX_NEARBY_ENEMIES);

    if (mWorld->HasComponentPool<EnemyMovementComponent>() && mWorld->HasComponentPool<TransformComponent>())
    {
        for (const Entity& other : mWorld->GetEntitiesWithComponents<EnemyMovementComponent, TransformComponent>())
        {
            if (!other.IsValid() || other == entity)
                continue;

            const TransformComponent* otherTransform = mWorld->GetComponent<TransformComponent>(other);
            if (!otherTransform)
                continue;

            Vec3 rel = otherTransform->mLocalPosition - myPos;
            rel.y = 0.0f;
            const float dist = std::sqrt(rel.LengthSquared());
            if (dist > ONNX_SENSE_RADIUS)
                continue;

            nearby.push_back({ rel.x / scale, rel.z / scale, dist / scale });
        }
    }

    std::sort(nearby.begin(), nearby.end(), [](const NearbyEnemyObs& lhs, const NearbyEnemyObs& rhs)
        {
            return lhs.dist < rhs.dist;
        });

    for (size_t idx = 0; idx < MAX_NEARBY_ENEMIES; ++idx)
    {
        const size_t base = 11 + idx * 4;
        if (idx < nearby.size())
        {
            input[base + 0] = nearby[idx].relX;
            input[base + 1] = nearby[idx].relZ;
            input[base + 2] = nearby[idx].dist;
            input[base + 3] = 0.0f;
        }
        else
        {
            input[base + 0] = 0.0f;
            input[base + 1] = 0.0f;
            input[base + 2] = 0.0f;
            input[base + 3] = 0.0f;
        }
    }

    Vec3 flatDelta = playerPos - myPos;
    flatDelta.y = 0.0f;
    const bool goalInSense = flatDelta.LengthSquared() <= ONNX_SENSE_RADIUS * ONNX_SENSE_RADIUS;
    const bool sensorEmpty = nearby.empty() && !goalInSense;
    input[23] = sensorEmpty ? 1.0f : 0.0f;
    if (sensorEmpty)
        return false;

    AIManager::OutputArray output{};
    std::wstring errorMessage;
    if (!AIMANAGER.RunModel(L"base_move", input, output, &errorMessage))
    {
        static bool loggedFailure = false;
        if (!loggedFailure)
        {
            std::wcerr << L"[EnemySystem] ONNX base_move inference failed: " << errorMessage << std::endl;
            loggedFailure = true;
        }
        return false;
    }

    const Vec2 action(
        (std::clamp)(output[0], -1.0f, 1.0f),
        (std::clamp)(output[1], -1.0f, 1.0f));

    Vec3 targetOffset(action.x * ONNX_TACTICAL_TARGET_RADIUS, 0.0f, action.y * ONNX_TACTICAL_TARGET_RADIUS);
    outTarget = myPos + targetOffset;
    outTarget.y = playerPos.y;
    return true;
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
