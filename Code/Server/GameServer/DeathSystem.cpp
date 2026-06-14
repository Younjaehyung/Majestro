#include "pch.h"
#include "DeathSystem.h"

#include "World.h"
#include "EventManager.h"
#include "GameEvents.h"
#include "PhysicsWorld.h"

#include "TransformComponent.h"
#include "GravityComponent.h"
#include "MovementComponent.h"
#include "PlayerComponent.h"
#include "EnemyComponent.h"
#include "HealthComponent.h"
#include "ArmorComponent.h"
#include "GameTimer.h"

DeathSystem::DeathSystem(World* world)  : System(world)
{
    mPhase = SysPhase::Sim;
}

void DeathSystem::Update(float deltaTime)
{
    (void)deltaTime;

    const float now = GetServerTotalTimeSeconds();
    mEventManager = mWorld->GetEventManager();


    if (mWorld->HasComponentPool<MainPlayerComponent>())
    {
        for (auto& entity : mWorld->GetEntitiesWithComponent<MainPlayerComponent>())
        {
            MainPlayerComponent* player = mWorld->GetComponent<MainPlayerComponent>(entity);
            if (!player || !player->IsDeathActive())
                continue;

            if (player->mDeathEndTime > 0.0f && now >= player->mDeathEndTime)   // 리스폰
            {
                if (!player->mRespawnVfxPending)
                {
                    // 리스폰 위치에 스폰 VFX를 랜더링
                    BeginRespawnVfx(entity);
                    player->mRespawnVfxPending = true;                      // 실제 부활을 RESPAWN_VFX_LEAD초 뒤로 예약
                    player->mRespawnReviveTime = now + RESPAWN_VFX_LEAD;
                }
                else if (now >= player->mRespawnReviveTime)
                {
                    // VFX가 자리잡은 뒤 실제 부활.
                    CompletePlayerDeath(entity);
                }
            }
        }
    }

    ConsumeEvent();


}

void DeathSystem::ConsumeEvent()
{
    BeginNormalDeath();

    BeginFallDeath();
}

void DeathSystem::BeginFallDeath()
{

    if (!mWorld->HasComponentPool<GravityComponent>())
        return;


    for (auto& entity : mWorld->GetEntitiesWithComponent<GravityComponent>())
    {
        TransformComponent* transform = mWorld->GetComponent<TransformComponent>(entity);
        if (!transform)
            continue;

        if (transform->mLocalPosition.y >= KILL_Y)
            continue;

        if (MainPlayerComponent* player = mWorld->GetComponent<MainPlayerComponent>(entity))
        {
            if (!player->IsDeathActive())
                BeginPlayerDeath(mWorld, entity, PlayerDeathCause::Fall, RESPAWN_DELAY, true);
            continue;
        }

        if (mWorld->GetComponent<EnemyComponent>(entity))
        {
            HealthComponent* health = mWorld->GetComponent<HealthComponent>(entity);
            if (health && health->IsDead())
                continue;

            if (mEventManager)
            {
                int32 lethal = 1;
                if (health)
                    lethal = health->mMaxHp + 1;
                if (ArmorComponent* armor = mWorld->GetComponent<ArmorComponent>(entity))
                    lethal += armor->mMaxArmor;

                EvDamage damage{};
                damage.target = entity;
                damage.amount = lethal;
                damage.instigator = Entity{};
                mEventManager->Enqueue<EvDamage>(damage);
            }
        }
    }
}

void DeathSystem::BeginNormalDeath()
{
    if (mEventManager)
    {
        mEventManager->Consume<EvPlayerDeathRequest>([&](const EvPlayerDeathRequest& e) {

            BeginPlayerDeath(mWorld, e.target, static_cast<PlayerDeathCause>(e.cause), e.holdSeconds, e.healthZeroEvent);
         });
    }
}

void DeathSystem::CompletePlayerDeath(Entity entity)
{
    MainPlayerComponent* player = mWorld->GetComponent<MainPlayerComponent>(entity);
    if (!player)
        return;

    if (player->mDeathCause == PlayerDeathCause::Fall)
        RespawnPlayer(entity);
    else
        RevivePlayerAtCurrentPosition(entity);
}

void DeathSystem::BeginRespawnVfx(Entity entity)
{
    //if (!mEventManager)
    //    return;

    //MainPlayerComponent* player = mWorld->GetComponent<MainPlayerComponent>(entity);
    //if (!player)
    //    return;

    //TransformComponent* transform = mWorld->GetComponent<TransformComponent>(entity);

    //// 리스폰될 위치 계산: 낙사는 스폰 지점(지형 높이 스냅), 일반 사망은 현재 위치(제자리 부활).
    //// RespawnPlayer()의 위치 산정과 동일하게 맞춰 VFX가 실제 부활 지점에 뜨도록 한다.
    //Vec3 pos = transform ? transform->mLocalPosition : Vec3::Zero;
    //if (player->mDeathCause == PlayerDeathCause::Fall)
    //{
    //    pos = player->mSpawnPosition;
    //    if (auto& physicsWorld = mWorld->GetPhysicsWorld())
    //    {
    //        float ground = 0.0f;
    //        if (physicsWorld->TryQueryTerrainHeightNear(pos, pos.y, 2000.0f, 5000.0f, ground))
    //            pos.y = ground;
    //    }
    //}

    //// 클라이언트로 방송되는 스폰 연출 VFX.
    //// reason=Respawn 이면 클라 VfxSystem::ResolveVfxSpawn 이 바람 필드(VFX_Wind_Energy_Field)를 재생한다.
    //EvEffectSpawn fx{};
    //fx.effectType = 0;                       // Respawn reason 에서는 사용하지 않음
    //fx.x = pos.x;
    //fx.y = pos.y;
    //fx.z = pos.z;
    //fx.reason = EffectSpawnReason::Respawn;
    //mEventManager->Enqueue<EvEffectSpawn>(fx);
}

void DeathSystem::RespawnPlayer(Entity entity)
{
    TransformComponent* transform = mWorld->GetComponent<TransformComponent>(entity);
    MainPlayerComponent* player = mWorld->GetComponent<MainPlayerComponent>(entity);
    if (!transform || !player)
        return;

    Vec3 pos = player->mSpawnPosition;

    if (auto& physicsWorld = mWorld->GetPhysicsWorld())
    {
        float ground = 0.0f;
        if (physicsWorld->TryQueryTerrainHeightNear(pos, pos.y, 100.0f, 100.0f, ground))
            pos.y = ground;
    }

    transform->mLocalPosition = pos;
    transform->mWorldMatrix = Matrix::CreateTranslation(pos);
    transform->mMovingVector = Vec3::Zero;

    if (GravityComponent* gravity = mWorld->GetComponent<GravityComponent>(entity))
    {
        gravity->mHight = pos.y;
        gravity->mGround = pos.y;
        gravity->mGravity = 0.0f;
        gravity->mFalling = false;
        gravity->mDropping = false;
        gravity->mGroundGraceLeft = 0.0f;
    }

    RevivePlayerAtCurrentPosition(entity);
}

void DeathSystem::RevivePlayerAtCurrentPosition(Entity entity)
{
    TransformComponent* transform = mWorld->GetComponent<TransformComponent>(entity);
    MainPlayerComponent* player = mWorld->GetComponent<MainPlayerComponent>(entity);
    if (!player)
        return;

    // 일반 사망은 사망 위치에서 대기 시간이 지난 뒤 같은 위치에서 조작 가능 상태로 되돌린다.
    player->mDeathCause = PlayerDeathCause::None;
    player->mDeathStartTime = 0.0f;
    player->mDeathEndTime = 0.0f;
    player->mRespawnVfxPending = false;
    player->mRespawnReviveTime = 0.0f;
    ClearFlag(player->mFlags, FLAG_DEAD);
    player->mPendingAction = PendingAction::None;
    player->mSpeed = 0.0f;
    player->mDash = false;
    player->mFalling = false;
    player->mHasMoveInput = false;
    player->mCanControlHorizontal = true;
    player->mCanControlVertical = true;
    player->mExternalMoveMode = static_cast<uint8>(ReplicatedExternalMoveMode::None);
    player->mExternalVelocity = Vec3::Zero;
    player->mExternalMoveEndTime = 0.0f;
    player->mFsm.ChangeState(player, IdleState::Instance());

    if (transform)
        transform->mMovingVector = Vec3::Zero;

    if (PlayerMovementComponent* movement = mWorld->GetComponent<PlayerMovementComponent>(entity))
        movement->mMovingDirection = Vec3::Zero;

    if (GravityComponent* gravity = mWorld->GetComponent<GravityComponent>(entity))
    {
        // Transform Y는 렌더 보정값을 포함하므로 중력 높이는 기존 convention에 맞춰 3을 뺀다.
        const float height = transform ? transform->mLocalPosition.y - 3.0f : gravity->mHight;
        gravity->mHight = height;
        gravity->mGround = height;
        gravity->mGravity = 0.0f;
        gravity->mFalling = false;
        gravity->mDropping = false;
        gravity->mGroundGraceLeft = 0.0f;
    }

    if (HealthComponent* health = mWorld->GetComponent<HealthComponent>(entity))
    {
        health->mCurrentHp = health->mMaxHp;

        if (auto eventManager = mWorld->GetEventManager())
        {
            EvHealthChanged event{};
            event.target = entity;
            event.currentHp = health->mCurrentHp;
            event.maxHp = health->mMaxHp;
            eventManager->Enqueue<EvHealthChanged>(event);
        }
    }
}


bool DeathSystem::BeginPlayerDeath(World* world, Entity entity, PlayerDeathCause cause, float holdSeconds, bool sendHealthZeroEvent)
{
    if (!world)
        return false;

    MainPlayerComponent* player = world->GetComponent<MainPlayerComponent>(entity);
    if (!player || player->IsDeathActive())
        return false;

    const float now = GetServerTotalTimeSeconds();

    // 사망 진입 시 이동과 입력 관련 잔여 상태를 모두 지워 Dead 상태만 복제되게 한다.
    player->mDeathCause = cause;
    player->mDeathStartTime = now;
    player->mDeathEndTime = now + holdSeconds;
    player->mRespawnVfxPending = false;
    player->mRespawnReviveTime = 0.0f;
    SetFlag(player->mFlags, FLAG_DEAD);
    player->mPendingAction = PendingAction::None;
    player->mSpeed = 0.0f;
    player->mDash = false;
    player->mFalling = false;
    player->mHasMoveInput = false;
    player->mExternalMoveMode = static_cast<uint8>(ReplicatedExternalMoveMode::None);
    player->mExternalVelocity = Vec3::Zero;
    player->mExternalMoveEndTime = 0.0f;
    player->mFsm.ChangeState(player, DeadState::Instance());

    if (TransformComponent* transform = world->GetComponent<TransformComponent>(entity))
        transform->mMovingVector = Vec3::Zero;

    if (PlayerMovementComponent* movement = world->GetComponent<PlayerMovementComponent>(entity))
        movement->mMovingDirection = Vec3::Zero;

    if (GravityComponent* gravity = world->GetComponent<GravityComponent>(entity))
    {
        gravity->mGravity = 0.0f;
        gravity->mFalling = false;
        gravity->mDropping = false;
        gravity->mGround = gravity->mHight;
        gravity->mGroundGraceLeft = 0.0f;
    }

    if (sendHealthZeroEvent)
    {
        if (HealthComponent* health = world->GetComponent<HealthComponent>(entity))
        {
            health->mCurrentHp = 0;
            if (auto eventManager = world->GetEventManager())
            {
                EvHealthChanged event{};
                event.target = entity;
                event.currentHp = 0;
                event.maxHp = health->mMaxHp;
                eventManager->Enqueue<EvHealthChanged>(event);
            }
        }
    }

    return true;
}
