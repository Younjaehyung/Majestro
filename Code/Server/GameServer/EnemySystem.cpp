#include "pch.h"
#include "EnemySystem.h"

#include "GameCore.h"
#include "ServerCore.h"
#include "Prefab.h"
#include "ResourceManager.h"
#include "NavMeshLoader.h"

#include "NetEntityComponent.h"
#include "TransformComponent.h"
#include "MovementComponent.h"
#include "FlyComponent.h"

#include "BeatSystem.h"
#include "EnemyComponent.h"
#include "ArmorComponent.h"
#include "BuffComponent.h"
#include "HealthComponent.h"
#include "GravityComponent.h"
#include "PhysicsWorld.h"
#include "EventManager.h"
#include "GameEvents.h"
#include "GameTimer.h"
#include "PlayerComponent.h"

namespace
{
    float FlatDistanceSquared(const Vec3& a, const Vec3& b)
    {
        const float dx = a.x - b.x;
        const float dz = a.z - b.z;
        return dx * dx + dz * dz;
    }

    bool HasDirectPath(World* world, const Vec3& start, const Vec3& end)
    {
        if (!world || !world->GetPhysicsWorld())
            return true;

        JoltStaticHit hit{};
        return !world->GetPhysicsWorld()->RayCastStatic(start, end, hit);
    }

    // 사망(또는 사망 처리 중) 플레이어는 타게팅 대상에서 제외
    bool IsPlayerDead(World* world, Entity playerEntity)
    {
        if (!world)
            return false;

        if (MainPlayerComponent* playerComp = world->GetComponent<MainPlayerComponent>(playerEntity))
        {
            if (playerComp->IsDeathActive())
                return true;
        }
        if (HealthComponent* health = world->GetComponent<HealthComponent>(playerEntity))
        {
            if (health->mCurrentHp <= 0)
                return true;
        }
        return false;
    }

    SkillType GetBrassSkillType(uint8 pattern)
    {
        switch (pattern)
        {
        case 0: return SkillType::BrassSkill1;
        case 1: return SkillType::BrassSkill2;
        case 2: return SkillType::BrassSkill3;
        case 3: return SkillType::BrassSkill4;
        default: return SkillType::BrassSkill1;
        }
    }

    EnemyAnimState GetBrassAnimState(uint8 pattern)
    {
        switch (pattern)
        {
        case 0: return EnemyAnimState::BrassAttack1;
        case 1: return EnemyAnimState::BrassAttack2;
        case 2: return EnemyAnimState::BrassAttack3;
        case 3: return EnemyAnimState::BrassAttack4;
        default: return EnemyAnimState::BrassAttack1;
        }
    }

    void BroadcastEnemySpawn(World* world, Entity spawnedEntity)
    {
        if (!world || !spawnedEntity.IsValid())
            return;

        NetEntityComponent* netComp = world->GetComponent<NetEntityComponent>(spawnedEntity);
        EnemyComponent* enemyComp = world->GetComponent<EnemyComponent>(spawnedEntity);
        if (!netComp || !enemyComp)
            return;

        S2C_SpawnPacekt spawnPkt(0, netComp->mNetEntityId, PrefabType::ENEMY);
        spawnPkt.isLocalPlayer = 0;
        spawnPkt.Type = enemyComp->mEnemyType;

        if (TransformComponent* transform = world->GetComponent<TransformComponent>(spawnedEntity))
        {
            spawnPkt.hasInitialTransform = 1;
            spawnPkt.x = transform->mLocalPosition.x;
            spawnPkt.y = transform->mLocalPosition.y;
            spawnPkt.z = transform->mLocalPosition.z;
        }

        for (auto playerEntity : world->GetEntitiesWithComponents<NetEntityComponent, MainPlayerComponent>())
        {
            NetEntityComponent* playerNetComp = world->GetComponent<NetEntityComponent>(playerEntity);
            if (!playerNetComp || playerNetComp->mSessionId == 0)
                continue;

            SendRequest request{ playerNetComp->mSessionId, PKT_Type::S2C_PKT_SPAWN, sizeof(S2C_SpawnPacekt) };
            request.StoreAs<S2C_SpawnPacekt>(spawnPkt);
            gSendQueue.Push(request);
        }

        if (auto eventManager = world->GetEventManager())
        {
            if (HealthComponent* hp = world->GetComponent<HealthComponent>(spawnedEntity))
            {
                EvHealthChanged evt{};
                evt.target = spawnedEntity;
                evt.currentHp = hp->mCurrentHp;
                evt.maxHp = hp->mMaxHp;
                eventManager->Enqueue<EvHealthChanged>(evt);
            }
        }
    }

    void SpawnEnemyAroundAndBroadcast(World* world, Entity owner, EnemyType enemyType, const Vec3& offset)
    {
        if (!world || !owner.IsValid())
            return;

        TransformComponent* ownerTransform = world->GetComponent<TransformComponent>(owner);
        if (!ownerTransform)
            return;

        InputCommand spawnCmd{};
        spawnCmd.SessionId = 0;
        spawnCmd.Type = PKT_Type::S2C_PKT_SPAWN;
        spawnCmd.StoreAs(EnemySpawnContext{ static_cast<uint8>(enemyType) });

        Entity spawned = PrefabFactory::Spawn(world, PrefabType::ENEMY, spawnCmd);
        if (!spawned.IsValid())
            return;

        Vec3 spawnPos = ownerTransform->mLocalPosition + offset;
        if (auto& physicsWorld = world->GetPhysicsWorld())
        {
            float ground = 0.0f;
            if (physicsWorld->TryQueryTerrainHeightNear(spawnPos, spawnPos.y, 100.0f, 100.0f, ground))
                spawnPos.y = ground;
        }

        if (TransformComponent* spawnedTransform = world->GetComponent<TransformComponent>(spawned))
        {
            spawnedTransform->mLocalPosition = spawnPos;
            spawnedTransform->mWorldMatrix = Matrix::CreateTranslation(spawnPos);
        }
        if (GravityComponent* gravity = world->GetComponent<GravityComponent>(spawned))
        {
            gravity->mGround = spawnPos.y;
            gravity->mHight = spawnPos.y;
            gravity->mGravity = 0.0f;
            gravity->mFalling = false;
            gravity->mDropping = false;
            gravity->mGroundGraceLeft = 0.0f;
        }
        if (FlyComponent* fly = world->GetComponent<FlyComponent>(spawned))
            fly->mGround = spawnPos.y;

        BroadcastEnemySpawn(world, spawned);
    }
}



EnemySystem::EnemySystem(World* world) : System(world)
{
}

void EnemySystem::Initialize()
{
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

    // 프레임 별 플레이어 위치 목록을 미리 수집 (길찾기 목표용).
    mPlayerPositions.clear();
    if (mWorld->HasComponentPool<PlayerMovementComponent>())
    {
        for (auto& playerEntity : mWorld->GetEntitiesWithComponent<PlayerMovementComponent>())
        {
            TransformComponent* tf = transformPool.GetComponent(playerEntity.GetID());
            if (!tf) continue;

            // 사망 플레이어는 길찾기 목표에서 제외 (생존자로 타게팅 전환).
            if (IsPlayerDead(mWorld, playerEntity)) continue;

            PlayerMovementComponent* pmc = mWorld->GetComponent<PlayerMovementComponent>(playerEntity);
            if (pmc && pmc->mNavPositionValid)
                mPlayerPositions.push_back(pmc->mNavPosition);
            else
                mPlayerPositions.push_back(tf->mLocalPosition);
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
        const Vec3 playerPos  = PathFinder(myPos, true);
        const std::vector<Entity> nearbyEnemies = mWorld->GetPhysicsWorld()
            ? mWorld->GetPhysicsWorld()->FindNearbyEnemies(entity, NEARBY_ENEMY_RADIUS, MAX_NEARBY_ENEMIES)
            : std::vector<Entity>{};

        EnemyComponent* enemyComp = mWorld->GetComponent<EnemyComponent>(entity);
        HealthComponent* enemyHealthComp = mWorld->GetComponent<HealthComponent>(entity);
        if (enemyComp == nullptr) { ++entityIndex; continue; }
        mc->mMovingSpeed = enemyComp->mSpeed;


        if (enemyHealthComp &&
            enemyHealthComp->mCurrentHp <= 0 &&
            enemyComp->mEnemyType != EnemyType::Obelisk)
        {
            HaltByState(enemyComp, mc, EnemyAnimState::Dead);
            ++entityIndex;
            continue;
        }

        if (enemyComp->mEnemyType == EnemyType::Obelisk)
        {
            HaltByState(enemyComp, mc, EnemyAnimState::Idle);

            auto clearSilenceFromLinkedPlayer = [&]()
            {
                if (!enemyComp->mLinkedPlayer.IsValid())
                    return;

                if (BuffComponent* targetBuff = mWorld->GetComponent<BuffComponent>(enemyComp->mLinkedPlayer))
                    targetBuff->RemoveBuff(BuffType::Silence, entity);

                enemyComp->mLinkedPlayer = Entity{};
            };

            if (enemyComp->mLinkedPlayer.IsValid())
            {
                MainPlayerComponent* linkedPlayer = mWorld->GetComponent<MainPlayerComponent>(enemyComp->mLinkedPlayer);
                HealthComponent* linkedHealth = mWorld->GetComponent<HealthComponent>(enemyComp->mLinkedPlayer);
                if (!linkedPlayer || (linkedPlayer->IsDeathActive()) || (linkedHealth && linkedHealth->mCurrentHp <= 0))
                    clearSilenceFromLinkedPlayer();
            }

            if (enemyHealthComp && enemyHealthComp->mCurrentHp <= 0)
            {
                clearSilenceFromLinkedPlayer();
            }
            else if (enemyHealthComp &&
                enemyHealthComp->mCurrentHp >= enemyHealthComp->mMaxHp &&
                !enemyComp->mLinkedPlayer.IsValid())
            {
                Entity selectedPlayer{};
                float bestDistSq = (std::numeric_limits<float>::max)();
                for (auto& playerEntity : mWorld->GetEntitiesWithComponent<PlayerMovementComponent>())
                {
                    MainPlayerComponent* playerComp = mWorld->GetComponent<MainPlayerComponent>(playerEntity);
                    TransformComponent* playerTf = transformPool.GetComponent(playerEntity.GetID());
                    HealthComponent* playerHealth = mWorld->GetComponent<HealthComponent>(playerEntity);
                    if (!playerComp || !playerTf || playerComp->IsDeathActive() || (playerHealth && playerHealth->mCurrentHp <= 0))
                        continue;

                    const float distSq = Vec3::DistanceSquared(myPos, playerTf->mLocalPosition);
                    if (distSq < bestDistSq)
                    {
                        bestDistSq = distSq;
                        selectedPlayer = playerEntity;
                    }
                }

                if (selectedPlayer.IsValid())
                {
                    if (BuffComponent* targetBuff = mWorld->GetComponent<BuffComponent>(selectedPlayer))
                    {
                        BuffData silence{};
                        silence.mKind = EffectKind::Debuff;
                        silence.mType = BuffType::Silence;
                        silence.mDurationPolicy = DurationPolicy::UntilSignal;
                        silence.mExecutionType = BuffExecutionType::Persistent;
                        silence.mSource = entity;
                        targetBuff->AddOrRefresh(silence);
                        enemyComp->mLinkedPlayer = selectedPlayer;
                    }
                }
            }

            if (eventManager && enemyHealthComp &&
                enemyHealthComp->mCurrentHp < enemyHealthComp->mMaxHp &&
                now >= enemyComp->mNextUtilityTime)
            {
                const int32 beforeHp = enemyHealthComp->mCurrentHp;
                enemyHealthComp->mCurrentHp = (std::min)(enemyHealthComp->mMaxHp, enemyHealthComp->mCurrentHp + enemyComp->mUtilityAmount);
                enemyComp->mNextUtilityTime = now + Beat * enemyComp->mUtilityIntervalBeats;

                if (beforeHp != enemyHealthComp->mCurrentHp)
                {
                    eventManager->Enqueue<EvHealthChanged>({
                        entity,
                        enemyHealthComp->mCurrentHp,
                        enemyHealthComp->mMaxHp
                    });
                }
            }

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

            // 사망 플레이어는 공격 범위 판단에서도 제외.
            if (IsPlayerDead(mWorld, playerEntity))
                continue;

            const float distSq = Vec3::DistanceSquared(myPos, playerTf->mLocalPosition);
            nearestPlayerDistSq = (std::min)(nearestPlayerDistSq, distSq);
        }

        if (enemyComp->mEnemyType == EnemyType::Fly)
        {
            if (FlyComponent* flyComp = mWorld->GetComponent<FlyComponent>(entity))
            {
                const float retreatDistSq = flyComp->mRetreatDistance * flyComp->mRetreatDistance;
                if (flyComp->mRetreating && nearestPlayerDistSq >= retreatDistSq)
                    flyComp->mRetreating = false;
            }
        }

        EnemyAnimState currentState = EnemyAnimState::Run;
        const bool pianoRushEnding =
            enemyComp->mEnemyType == EnemyType::Pianoman &&
            enemyComp->mRushEndAnimEndTime > now;
        const bool brassRushEnding =
            enemyComp->mEnemyType == EnemyType::Brass &&
            enemyComp->mBrassRushEndHoldUntilTime > now;
        const bool bongomanCommittedAttack =
            enemyComp->mEnemyType == EnemyType::Bongoman &&
            enemyComp->mPendingAttackTime >= 0.0f;
        bool brassFacingReady = false;
        if (enemyComp->mEnemyType == EnemyType::Brass)
        {
            Vec3 toPlayer = playerPos - myPos;
            toPlayer.y = 0.0f;
            if (toPlayer.LengthSquared() > 1e-8f)
            {
                toPlayer.Normalize();
                Vec3 look = tf->GetLook();
                look.y = 0.0f;
                if (look.LengthSquared() > 1e-8f)
                {
                    look.Normalize();
                    constexpr float kBrassAttackEnterFacingDot = 0.94f;
                    brassFacingReady = look.Dot(toPlayer) >= kBrassAttackEnterFacingDot;
                }
            }
        }
        if (pianoRushEnding)
            currentState = EnemyAnimState::Idle;
        else if (brassRushEnding)
            currentState = (enemyComp->mBrassAttackPattern == 2)
                ? EnemyAnimState::Idle
                : EnemyAnimState::Attack;
        else if (enemyComp->mEnemyType == EnemyType::Brass)
            currentState = (brassFacingReady && (now >= enemyComp->mNextAttackTime ||
                enemyComp->mPendingSkillType != 0 ||
                now <= enemyComp->mAttackAnimEndTime))
                ? EnemyAnimState::Attack
                : EnemyAnimState::Run;
        else if (nearestPlayerDistSq <= enemyComp->AttackRangeSq || bongomanCommittedAttack)
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
                Vec3 shieldForward = tf->GetLook();
                shieldForward.y = 0.0f;
                if (shieldForward.LengthSquared() > 1e-6f)
                    shieldForward.Normalize();
                else
                    shieldForward = Vec3::Forward;

                const Vec3 shieldEffectPos = myPos + shieldForward * 300.0f;
                const float shieldYawDeg = DirectX::XMConvertToDegrees(std::atan2(shieldForward.x, shieldForward.z));
                eventManager->Enqueue<EvArmorChanged>({ entity, armorComp->mCurrentArmor, armorComp->mMaxArmor });
                eventManager->Enqueue<EvEffectSpawn>({
                    static_cast<uint8>(SkillType::BongoShild),
                    shieldEffectPos.x,
                    shieldEffectPos.y,
                    shieldEffectPos.z,
                    EffectSpawnReason::Fire,
                    0.0f,
                    shieldYawDeg,
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

        HandleRunState(entity, enemyComp, mc, myPos, playerPos, navSystem, now, dt, entityIndex);

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
    if (enemyComp->mEnemyType != EnemyType::Brass &&
        nearestPlayerDistSq > enemyComp->AttackRangeSq &&
        !bongomanCommittedAttack)
    {
        enemyComp->mPendingAttackTime = -1.0f;
        return false;
    }

    Vec3 myPos = Vec3::Zero;
    Vec3 playerPos = Vec3::Zero;
    if (TransformComponent* tf = mWorld->GetComponent<TransformComponent>(entity))
    {
        myPos = tf->mLocalPosition;
        playerPos = PathFinder(myPos, true);
    }

    if (enemyComp->mEnemyType == EnemyType::Brass)
    {
        constexpr float kBrassWakeRange = 2000.0f;
        constexpr float kBrassWakeRangeSq = kBrassWakeRange * kBrassWakeRange;

        if (!enemyComp->mBossEncounterActivated)
        {
            if (nearestPlayerDistSq <= kBrassWakeRangeSq)
            {
                enemyComp->mBossEncounterActivated = true;
            }
            else
            {
                movementComp->mMovingDirection = Vec3::Zero;
                movementComp->mPathCount = 0;
                movementComp->mPathIndex = 0;
                enemyComp->mAnimState = static_cast<uint8>(EnemyAnimState::Idle);
                return false;
            }
        }
    }

    switch (enemyComp->mEnemyType)
    {
    case EnemyType::HornMan:
    case EnemyType::Obelisk:
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
    case EnemyType::Slime:
        movementComp->mMovingDirection = Vec3::Zero;
        movementComp->mPathCount = 0;
        movementComp->mPathIndex = 0;

        if (eventManager && enemyComp->mNextAttackTime <= nowSeconds)
        {
            eventManager->Enqueue<EvMeleeAttackRequest>({ entity, SkillType::SlimeAttack });
            enemyComp->mNextAttackTime = nowSeconds + beatSeconds * enemyComp->mAttackCool;
            enemyComp->mAttackAnimEndTime = nowSeconds + enemyComp->mAttackAnimTime;
        }
        break;
	    case EnemyType::Brass:
	    {
	        auto getBrassRushEndDuration = [&]()
	        {
	            return beatSeconds * enemyComp->mBrassAttackCool[enemyComp->mBrassAttackPattern];
	        };
            const bool brassHoldKeepsAttackAnim = enemyComp->mBrassAttackPattern != 2;

	        if (enemyComp->mBrassRushEndHoldUntilTime > nowSeconds)
	        {
	            movementComp->mMovingDirection = Vec3::Zero;
                movementComp->mPathCount = 0;
                movementComp->mPathIndex = 0;
                enemyComp->mAnimState = static_cast<uint8>(
                    brassHoldKeepsAttackAnim
                    ? GetBrassAnimState(enemyComp->mBrassAttackPattern)
                    : EnemyAnimState::Idle);
                break;
            }

        const bool brassSkill3Active =
            enemyComp->mPendingSkillType == static_cast<uint8>(SkillType::BrassSkill3) &&
            enemyComp->mBrassSkill3ShotsRemaining > 0;

        if (brassSkill3Active)
        {
            movementComp->mMovingSpeed = enemyComp->mSpeed * 1.2f;
            movementComp->mPathCount = 0;
            movementComp->mPathIndex = 0;

            Vec3 chaseDir = playerPos - myPos;
            chaseDir.y = 0.0f;
            if (chaseDir.LengthSquared() > 1e-8f)
            {
                chaseDir.Normalize();
                movementComp->mMovingDirection = chaseDir;
            }
            else
            {
                movementComp->mMovingDirection = Vec3::Zero;
            }

            if (eventManager && nowSeconds >= enemyComp->mBrassSkill3NextShotTime)
            {
            TransformComponent* brassTf = mWorld->GetComponent<TransformComponent>(entity);
            Vec3 baseDir = playerPos - myPos;
            baseDir.y = 0.0f;
            if (baseDir.LengthSquared() <= 1e-8f)
                baseDir = Vec3::Forward;
            else
                baseDir.Normalize();

            const int shotsFired = 8 - static_cast<int>(enemyComp->mBrassSkill3ShotsRemaining);
            const float startYawDeg = -70.0f;
            const float yawStepDeg = 45.0f;
            const float shotYawDeg = startYawDeg + yawStepDeg * shotsFired;
            const float shotYawRad = DirectX::XMConvertToRadians(shotYawDeg);
            const float cosYaw = std::cos(shotYawRad);
            const float sinYaw = std::sin(shotYawRad);

            Vec3 shotDir;
            shotDir.x = baseDir.x * cosYaw - baseDir.z * sinYaw;
            shotDir.y = 0.0f;
            shotDir.z = baseDir.x * sinYaw + baseDir.z * cosYaw;
            if (shotDir.LengthSquared() <= 1e-8f)
                shotDir = baseDir;
            else
                shotDir.Normalize();

            if (brassTf)
                brassTf->LookAt(shotDir);

            eventManager->Enqueue<EvRangedAttackRequest>({ entity, SkillType::BrassSkill3 });
                --enemyComp->mBrassSkill3ShotsRemaining;
                enemyComp->mBrassSkill3NextShotTime += enemyComp->mBrassSkill3ShotInterval;

	                if (enemyComp->mBrassSkill3ShotsRemaining == 0)
	                {
	                    enemyComp->mPendingAttackTime = -1.0f;
	                    enemyComp->mPendingSkillType = 0;
	                    enemyComp->mBrassRushEndHoldUntilTime = nowSeconds + getBrassRushEndDuration();
	                    enemyComp->mAnimState = static_cast<uint8>(EnemyAnimState::Idle);
	                }
	            }

            if (enemyComp->mBrassSkill3ShotsRemaining > 0)
                enemyComp->mAnimState = static_cast<uint8>(EnemyAnimState::BrassAttack3);
            break;
        }

        movementComp->mMovingDirection = Vec3::Zero;
        movementComp->mPathCount = 0;
        movementComp->mPathIndex = 0;

        if (eventManager &&
            enemyComp->mPendingSkillType == static_cast<uint8>(SkillType::BrassSkill2) &&
            enemyComp->mPendingAttackTime >= 0.0f &&
            nowSeconds >= enemyComp->mPendingAttackTime)
        {
            const Vec3 attackDir = playerPos - myPos;
            float attackYawDeg = 0.0f;
            if (attackDir.x * attackDir.x + attackDir.z * attackDir.z > 1e-8f)
                attackYawDeg = DirectX::XMConvertToDegrees(std::atan2(attackDir.x, attackDir.z));

	            eventManager->Enqueue<EvRangedAttackRequest>({ entity, SkillType::BrassSkill2 });
	            enemyComp->mAttackAnimEndTime = nowSeconds + enemyComp->mAttackAnimTime;
	            enemyComp->mPendingAttackTime = -1.0f;
	            enemyComp->mPendingSkillType = 0;
		            enemyComp->mBrassRushEndHoldUntilTime = nowSeconds + getBrassRushEndDuration();
		            enemyComp->mAnimState = static_cast<uint8>(GetBrassAnimState(enemyComp->mBrassAttackPattern));
		            break;
		        }

        if (eventManager && enemyComp->mNextAttackTime <= nowSeconds)
        {
            std::uniform_int_distribution<int> brassPick(3, 3);
	            const uint8 pattern = static_cast<uint8>(brassPick(RandomEngine()));
	            enemyComp->mBrassAttackPattern = pattern;

	            const SkillType brassSkill = GetBrassSkillType(pattern);
	            enemyComp->mAnimState = static_cast<uint8>(GetBrassAnimState(pattern));
	            enemyComp->mNextAttackTime = nowSeconds + beatSeconds * enemyComp->mAttackCool;
	            enemyComp->mBrassNextAttackTime[pattern] = enemyComp->mNextAttackTime;
	            enemyComp->mBrassSkill3ShotsRemaining = 0;
	            enemyComp->mBrassSkill3NextShotTime = 0.0f;
            enemyComp->mBrassSkill3ShotInterval = 0.0f;

            const Vec3 attackDir = playerPos - myPos;
            float attackYawDeg = 0.0f;
            if (attackDir.x * attackDir.x + attackDir.z * attackDir.z > 1e-8f)
                attackYawDeg = DirectX::XMConvertToDegrees(std::atan2(attackDir.x, attackDir.z));

            if (brassSkill == SkillType::BrassSkill2)
            {
                enemyComp->mPendingAttackTime = nowSeconds + beatSeconds;
                enemyComp->mPendingSkillType = static_cast<uint8>(brassSkill);
                enemyComp->mAttackAnimEndTime = enemyComp->mPendingAttackTime + enemyComp->mAttackAnimTime;
            }
            else if (brassSkill == SkillType::BrassSkill3)
            {
                enemyComp->mPendingAttackTime = nowSeconds;
                enemyComp->mPendingSkillType = static_cast<uint8>(brassSkill);
                enemyComp->mAttackAnimEndTime = nowSeconds + beatSeconds * 4.0f;
                enemyComp->mBrassSkill3ShotsRemaining = 8;
                enemyComp->mBrassSkill3ShotInterval = beatSeconds * 4.0f / 8.0f;
                enemyComp->mBrassSkill3NextShotTime = nowSeconds;
                eventManager->Enqueue<EvEffectSpawn>({
                    static_cast<uint8>(brassSkill),
                    myPos.x,
                    myPos.y,
                    myPos.z,
                    EffectSpawnReason::Fire,
                    0.0f,
                    attackYawDeg,
                    0.0f
                });
            }
            else
            {
                enemyComp->mPendingAttackTime = -1.0f;
                enemyComp->mPendingSkillType = 0;
                enemyComp->mAttackAnimEndTime = nowSeconds + enemyComp->mAttackAnimTime;

                if (brassSkill == SkillType::BrassSkill1)
                {
                    eventManager->Enqueue<EvEffectSpawn>({
                        static_cast<uint8>(brassSkill),
                        myPos.x,
                        myPos.y,
                        myPos.z,
                        EffectSpawnReason::Fire,
                        0.0f,
                        attackYawDeg,
                        0.0f
	                    });
	                    SpawnEnemyAroundAndBroadcast(mWorld, entity, EnemyType::Pianoman, Vec3(-350.0f, 0.0f, 250.0f));
	                    SpawnEnemyAroundAndBroadcast(mWorld, entity, EnemyType::Bongoman, Vec3(350.0f, 0.0f, 250.0f));
	                    SpawnEnemyAroundAndBroadcast(mWorld, entity, EnemyType::HornMan, Vec3(0.0f, 0.0f, -350.0f));
	                    enemyComp->mBrassRushEndHoldUntilTime = nowSeconds + getBrassRushEndDuration();
	                    enemyComp->mAnimState = static_cast<uint8>(GetBrassAnimState(enemyComp->mBrassAttackPattern));
	                }
	                else if (brassSkill == SkillType::BrassSkill4)
	                {
	                    eventManager->Enqueue<EvRangedAttackRequest>({ entity, SkillType::BrassSkill4 });
	                    enemyComp->mBrassRushEndHoldUntilTime = nowSeconds + getBrassRushEndDuration();
	                    enemyComp->mAnimState = static_cast<uint8>(GetBrassAnimState(enemyComp->mBrassAttackPattern));
	                }
	            }
        }
        break;
    }
    case EnemyType::Fly:
    {
        constexpr float kFlyMeleeRange = 180.0f;
        constexpr float kFlyArriveThresholdSq = 4.0f * 100.0f;
        FlyComponent* flyComp = mWorld->GetComponent<FlyComponent>(entity);
        if (flyComp)
        {
            flyComp->mDirectFlight = false;
            flyComp->mAttackDiveActive = false;
        }

        if (flyComp && flyComp->mRetreating)
        {
            const float retreatDistSq = flyComp->mRetreatDistance * flyComp->mRetreatDistance;
            if (nearestPlayerDistSq >= retreatDistSq)
            {
                flyComp->mRetreating = false;
                movementComp->mMovingDirection = Vec3::Zero;
                movementComp->mPathCount = 0;
                movementComp->mPathIndex = 0;
                return false;
            }

            movementComp->mMovingSpeed = enemyComp->mSpeed * flyComp->mRetreatSpeedMultiplier;
            flyComp->mDirectFlight = true;
            movementComp->mPathCount = 0;
            movementComp->mPathIndex = 0;

            Vec3 retreatDir = myPos - playerPos;
            const float desiredY = flyComp->mGround + flyComp->mHoverHeight + 3.0f;
            retreatDir.y = (std::max)(0.0f, desiredY - myPos.y);
            if (retreatDir.LengthSquared() > 1e-8f)
            {
                retreatDir.Normalize();
                movementComp->mMovingDirection = retreatDir;
            }
            else
            {
                movementComp->mMovingDirection = Vec3::Up;
            }
            break;
        }

        movementComp->mMovingSpeed = enemyComp->mSpeed * 1.15f;

        const bool flyAttackOnCooldown = enemyComp->mNextAttackTime > nowSeconds;
        if (flyAttackOnCooldown)
        {
            movementComp->mPathCount = 0;
            movementComp->mPathIndex = 0;
            movementComp->mMovingDirection = Vec3::Zero;
        }
        else
        {
            if (flyComp)
                flyComp->mAttackDiveActive = true;

            const Vec3 rushTarget = playerPos;
            auto navSystem = mWorld->GetNavSystem();
            Vec3 rayStart = myPos;
            Vec3 rayEnd = rushTarget;
            rayStart.y += 60.0f;
            rayEnd.y += 60.0f;
            const bool directPath = HasDirectPath(mWorld, rayStart, rayEnd);
            if (flyComp)
                flyComp->mDirectFlight = directPath;

            bool hasPathDir = false;
            Vec3 rushDir = Vec3::Zero;
            if (!directPath && navSystem && navSystem->IsInitialized())
            {
                bool ok = navSystem->FindPath(myPos, rushTarget, movementComp->mPath, movementComp->mPathCount, ENEMY_MAX_WAYPOINTS);
                if (!ok)
                {
                    movementComp->mPathCount = 1;
                    movementComp->mPath[0] = rushTarget;
                }

                movementComp->mPathIndex = 0;
                while (movementComp->mPathCount > 0 && movementComp->mPathIndex < movementComp->mPathCount - 1)
                {
                    Vec3 toWp = movementComp->mPath[movementComp->mPathIndex] - myPos;
                    toWp.y = 0.0f;
                    if (toWp.LengthSquared() < kFlyArriveThresholdSq)
                        ++movementComp->mPathIndex;
                    else
                        break;
                }

                if (movementComp->mPathCount > 0 && movementComp->mPathIndex < movementComp->mPathCount)
                {
                    rushDir = movementComp->mPath[movementComp->mPathIndex] - myPos;
                    rushDir.y = rushTarget.y - myPos.y;
                    hasPathDir = true;
                }
            }
            else if (directPath)
            {
                movementComp->mPathCount = 0;
                movementComp->mPathIndex = 0;
                rushDir = rushTarget - myPos;
                hasPathDir = true;
            }

            if (!hasPathDir)
            {
                rushDir = rushTarget - myPos;
            }

            if (rushDir.LengthSquared() > 1e-8f)
            {
                rushDir.Normalize();
                movementComp->mMovingDirection = rushDir;
            }
            else
            {
                movementComp->mMovingDirection = Vec3::Zero;
            }
        }

        if (eventManager && enemyComp->mNextAttackTime <= nowSeconds && nearestPlayerDistSq <= kFlyMeleeRange * kFlyMeleeRange)
        {
            eventManager->Enqueue<EvMeleeAttackRequest>({ entity, SkillType::PianoAttack });
            enemyComp->mNextAttackTime = nowSeconds + beatSeconds * enemyComp->mAttackCool;
            enemyComp->mAttackAnimEndTime = nowSeconds + enemyComp->mAttackAnimTime;
            if (flyComp)
                flyComp->mRetreating = true;
            Vec3 retreatDir = myPos - playerPos;
            if (flyComp)
            {
                const float desiredY = flyComp->mGround + flyComp->mHoverHeight + 3.0f;
                retreatDir.y = (std::max)(0.0f, desiredY - myPos.y);
            }
            if (retreatDir.LengthSquared() > 1e-8f)
            {
                retreatDir.Normalize();
                movementComp->mMovingDirection = retreatDir;
            }
            else
            {
                movementComp->mMovingDirection = Vec3::Up;
            }
            movementComp->mPathCount = 0;
            movementComp->mPathIndex = 0;
        }
        break;
    }
    case EnemyType::Pianoman:
    {
        constexpr float kPianoMeleeRange = 160.0f;
        constexpr float kPianoArriveThresholdSq = 4.0f * 100.0f;
        movementComp->mMovingSpeed = enemyComp->mSpeed * 1.2f;

        const bool pianoAttackOnCooldown = enemyComp->mNextAttackTime > nowSeconds;
        if (pianoAttackOnCooldown)
        {
            movementComp->mPathCount = 0;
            movementComp->mPathIndex = 0;
            movementComp->mMovingDirection = Vec3::Zero;
        }
        else
        {
            Vec3 rushTarget = playerPos;
            auto navSystem = mWorld->GetNavSystem();
            if (/*mUseOnnxBaseMove*/ false && navSystem)
            {
                Vec3 frontTarget = playerPos;
                if (TryComputeOnnxModelTarget(L"front", entity, myPos, playerPos, navSystem, frontTarget))
                    rushTarget = frontTarget;
            }

            bool hasPathDir = false;
            Vec3 rushDir = Vec3::Zero;
            if (navSystem && navSystem->IsInitialized())
            {
                bool ok = navSystem->FindPath(myPos, rushTarget, movementComp->mPath, movementComp->mPathCount, ENEMY_MAX_WAYPOINTS);
                if (!ok)
                {
                    movementComp->mPathCount = 1;
                    movementComp->mPath[0] = rushTarget;
                }

                movementComp->mPathIndex = 0;
                while (movementComp->mPathCount > 0 && movementComp->mPathIndex < movementComp->mPathCount - 1)
                {
                    Vec3 toWp = movementComp->mPath[movementComp->mPathIndex] - myPos;
                    toWp.y = 0.0f;
                    if (toWp.LengthSquared() < kPianoArriveThresholdSq)
                        ++movementComp->mPathIndex;
                    else
                        break;
                }

                if (movementComp->mPathCount > 0 && movementComp->mPathIndex < movementComp->mPathCount)
                {
                    rushDir = movementComp->mPath[movementComp->mPathIndex] - myPos;
                    rushDir.y = 0.0f;
                    hasPathDir = true;
                }
            }

            if (!hasPathDir)
            {
                rushDir = rushTarget - myPos;
                rushDir.y = 0.0f;
            }

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
            enemyComp->mRushEndAnimEndTime = nowSeconds + enemyComp->mRushEndAnimTime;
            movementComp->mMovingDirection = Vec3::Zero;
            movementComp->mPathCount = 0;
            movementComp->mPathIndex = 0;
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

    if (enemyComp->mEnemyType == EnemyType::Pianoman && enemyComp->mRushEndAnimEndTime > nowSeconds)
    {
        enemyComp->mAnimState = static_cast<uint8>(EnemyAnimState::Idle);
    }
    else if (enemyComp->mEnemyType == EnemyType::Pianoman)
    {
        const bool pianoAttackOnCooldown = enemyComp->mNextAttackTime > nowSeconds;
        enemyComp->mAnimState = static_cast<uint8>(pianoAttackOnCooldown ? EnemyAnimState::Idle : EnemyAnimState::Attack);
    }
    else if (enemyComp->mEnemyType == EnemyType::Fly)
    {
        enemyComp->mAnimState = static_cast<uint8>(
            nowSeconds <= enemyComp->mAttackAnimEndTime
            ? EnemyAnimState::Attack
            : EnemyAnimState::Idle);
    }
    else if (enemyComp->mEnemyType == EnemyType::Brass)
    {
        enemyComp->mAnimState = static_cast<uint8>(
            nowSeconds <= enemyComp->mAttackAnimEndTime
            ? GetBrassAnimState(enemyComp->mBrassAttackPattern)
            : EnemyAnimState::Idle);
    }
    else if (nowSeconds <= enemyComp->mAttackAnimEndTime)
    {
        enemyComp->mAnimState = static_cast<uint8>(EnemyAnimState::Attack);
    }
    else
    {
        enemyComp->mAnimState = static_cast<uint8>(EnemyAnimState::Idle);
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
    float nowSeconds,
    float dt,
    int entityIndex)
{
    if (!enemyComp || !movementComp)
        return;

    FlyComponent* flyComp = enemyComp->mEnemyType == EnemyType::Fly
        ? mWorld->GetComponent<FlyComponent>(entity)
        : nullptr;
    if (flyComp)
        flyComp->mDirectFlight = false;

    if (enemyComp->mEnemyType == EnemyType::Brass)
    {
        Vec3 toPlayer = playerPos - myPos;
        toPlayer.y = 0.0f;
        if (toPlayer.LengthSquared() <= enemyComp->AttackRangeSq)
        {
            movementComp->mMovingDirection = Vec3::Zero;
            movementComp->mPathCount = 0;
            movementComp->mPathIndex = 0;
            enemyComp->mAnimState = static_cast<uint8>(EnemyAnimState::Idle);
            return;
        }
    }

    Vec3 desiredTarget = playerPos;
    const bool allowOnnxBaseMove = mUseOnnxBaseMove && enemyComp->mEnemyType != EnemyType::Brass;
    if (allowOnnxBaseMove)
    {
        Vec3 onnxTarget = playerPos;
        if (TryComputeOnnxBaseMoveTarget(entity, myPos, playerPos, navSystem, onnxTarget))
            desiredTarget = onnxTarget;
    }

    UpdateDetourState(movementComp, myPos, desiredTarget, dt);

    if (movementComp->mDetourActive)
    {
        const bool movedFarEnough = FlatDistanceSquared(myPos, movementComp->mDetourStartPos) >= DETOUR_EXIT_DISTANCE * DETOUR_EXIT_DISTANCE;
        const bool timedOut = nowSeconds >= movementComp->mDetourEndTime;

        if (movedFarEnough || timedOut)
        {
            movementComp->mDetourActive = false;
            movementComp->mStuckElapsed = 0.0f;
            movementComp->mProgressSampleElapsed = 0.0f;
            movementComp->mProgressSamplePos = myPos;
        }
    }

    if (!movementComp->mDetourActive && ShouldEnterDetour(movementComp, myPos, desiredTarget))
    {
        movementComp->mDetourActive = true;
        movementComp->mDetourStartPos = myPos;
        movementComp->mDetourEndTime = nowSeconds + DETOUR_TIMEOUT_SECONDS;
        movementComp->mStuckElapsed = 0.0f;
    }

    if (movementComp->mDetourActive)
        desiredTarget = PathFinder(myPos, true);

    // ---- 재탐색 판단 ----
    movementComp->mPathTimer -= dt;

    const bool targetMoved = Vec3::DistanceSquared(desiredTarget, movementComp->mTarget) > RETARGET_THRESHOLD_SQ;
    const bool needRepath = (movementComp->mPathTimer <= 0.f) || targetMoved || (movementComp->mPathCount == 0);

    if (needRepath)
    {
        movementComp->mTarget = desiredTarget;

        bool ok = false;
        Vec3 rayStart = myPos;
        Vec3 rayEnd = desiredTarget;
        rayStart.y += 60.0f;
        rayEnd.y += 60.0f;
        const bool directPath = enemyComp->mEnemyType == EnemyType::Fly &&
            HasDirectPath(mWorld, rayStart, rayEnd);
        if (flyComp)
            flyComp->mDirectFlight = directPath;
        if (directPath)
        {
            movementComp->mPathCount = 0;
            movementComp->mPathIndex = 0;
            ok = true;
        }
        else if (navSystem && navSystem->IsInitialized())
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
            toWp.y = 0.0f;
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
            enemyComp->mAnimState = static_cast<uint8>(EnemyAnimState::Idle);
        }
    }
}

void EnemySystem::UpdateDetourState(
    EnemyMovementComponent* movementComp,
    const Vec3& myPos,
    const Vec3& desiredTarget,
    float dt)
{
    if (!movementComp)
        return;

    if (movementComp->mProgressSampleElapsed <= 0.0f)
        movementComp->mProgressSamplePos = myPos;

    movementComp->mProgressSampleElapsed += dt;
    if (movementComp->mProgressSampleElapsed < STUCK_PROGRESS_WINDOW)
        return;

    const float movedSq = FlatDistanceSquared(myPos, movementComp->mProgressSamplePos);
    const float targetDistSq = FlatDistanceSquared(myPos, desiredTarget);
    const bool hasMoveIntent = movementComp->mPathCount > 0 || movementComp->mMovingDirection.LengthSquared() > 1e-8f;
    const bool targetIsFar = targetDistSq >= STUCK_TARGET_DISTANCE * STUCK_TARGET_DISTANCE;

    if (hasMoveIntent && targetIsFar && movedSq < STUCK_PROGRESS_DISTANCE * STUCK_PROGRESS_DISTANCE)
        movementComp->mStuckElapsed += movementComp->mProgressSampleElapsed;
    else
        movementComp->mStuckElapsed = 0.0f;

    movementComp->mProgressSamplePos = myPos;
    movementComp->mProgressSampleElapsed = 0.0f;
}

bool EnemySystem::ShouldEnterDetour(
    const EnemyMovementComponent* movementComp,
    const Vec3& myPos,
    const Vec3& desiredTarget) const
{
    if (!movementComp)
        return false;

    if (movementComp->mStuckElapsed < STUCK_TRIGGER_SECONDS)
        return false;

    return FlatDistanceSquared(myPos, desiredTarget) >= STUCK_TARGET_DISTANCE * STUCK_TARGET_DISTANCE;
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
    const std::shared_ptr<Navigation>& navSystem,
    Vec3& outTarget) const
{
    return TryComputeOnnxModelTarget(L"base_move", entity, myPos, playerPos, navSystem, outTarget);
}

bool EnemySystem::TryComputeOnnxModelTarget(
    const std::wstring& modelKey,
    const Entity& entity,
    const Vec3& myPos,
    const Vec3& playerPos,
    const std::shared_ptr<Navigation>& navSystem,
    Vec3& outTarget) const
{
    if (!AIMANAGER.HasModel(modelKey))
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
    if (!AIMANAGER.RunModel(modelKey, input, output, &errorMessage))
    {
        static bool loggedFailure = false;
        if (!loggedFailure)
        {
            std::wcerr << L"[EnemySystem] ONNX " << modelKey << L" inference failed: " << errorMessage << std::endl;
            loggedFailure = true;
        }
        return false;
    }

    const Vec2 action(
        (std::clamp)(output[0], -1.0f, 1.0f),
        (std::clamp)(output[1], -1.0f, 1.0f));

    Vec3 targetOffset(action.x * ONNX_TACTICAL_TARGET_RADIUS, 0.0f, action.y * ONNX_TACTICAL_TARGET_RADIUS);
    outTarget = myPos + targetOffset;
    if (navSystem && navSystem->IsInitialized())
        outTarget.y = navSystem->GetHeightAtPosition(outTarget);
    else
        outTarget.y = myPos.y;
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
    movementComp->mDetourActive = false;
    movementComp->mStuckElapsed = 0.0f;
    movementComp->mProgressSampleElapsed = 0.0f;
}

Vec3 EnemySystem::PathFinder(const Vec3& from, bool preserveHeight)
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
    if (!preserveHeight)
        nearest.y = from.y;
    return nearest;
}
