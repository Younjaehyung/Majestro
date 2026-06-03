#include "pch.h"
#include "FallDeathSystem.h"

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

FallDeathSystem::FallDeathSystem(World* world)
    : System(world)
{
    mPhase = SysPhase::Sim;
}

void FallDeathSystem::Update(float deltaTime)
{
    (void)deltaTime;

    if (!mWorld->HasComponentPool<GravityComponent>())
        return;

    auto eventManager = mWorld->GetEventManager();

   
    for (auto& entity : mWorld->GetEntitiesWithComponent<GravityComponent>())
    {
        TransformComponent* transform = mWorld->GetComponent<TransformComponent>(entity);
        if (!transform)
            continue;

        if (transform->mLocalPosition.y >= KILL_Y)
            continue;

        // 플레이어
        if (MainPlayerComponent* player = mWorld->GetComponent<MainPlayerComponent>(entity))
        {
            const float now = GetServerTotalTimeSeconds();

            if (player->mIsDead)
            {
                // 리스폰
                if (now >= player->mRespawnTime)
                {
                    RespawnPlayer(entity);
                    player->mIsDead = false;
                }
                continue;
            }

            // 사망 연출 시작
            player->mIsDead = true;
            player->mRespawnTime = now + RESPAWN_DELAY;
            transform->mMovingVector = Vec3::Zero;

            if (GravityComponent* grav = mWorld->GetComponent<GravityComponent>(entity))
            {
                grav->mGravity = 0.0f;
                grav->mFalling = false;
                grav->mDropping = false;
                grav->mGround = grav->mHight;
                grav->mGroundGraceLeft = 0.0f;
            }
            if (PlayerMovementComponent* pmc = mWorld->GetComponent<PlayerMovementComponent>(entity))
                pmc->mMovingDirection = Vec3::Zero;

            // HP 0 으로 즉시 사망 표시
            if (HealthComponent* health = mWorld->GetComponent<HealthComponent>(entity))
            {
                health->mCurrentHp = 0;
                if (eventManager)
                {
                    EvHealthChanged evt{};
                    evt.target = entity;
                    evt.currentHp = 0;
                    evt.maxHp = health->mMaxHp;
                    eventManager->Enqueue<EvHealthChanged>(evt);
                }
            }
            continue;
        }

        // 적
        if (mWorld->GetComponent<EnemyComponent>(entity))
        {
            HealthComponent* health = mWorld->GetComponent<HealthComponent>(entity);

            if (health && health->IsDead())
                continue;

            if (eventManager)
            {
                int32 lethal = 1;
                if (health)
                    lethal = health->mMaxHp + 1;
                if (ArmorComponent* armor = mWorld->GetComponent<ArmorComponent>(entity))
                    lethal += armor->mMaxArmor;

                EvDamage dmg{};
                dmg.target = entity;
                dmg.amount = lethal;
                dmg.instigator = Entity{}; // 환경(무효 엔티티)
                eventManager->Enqueue<EvDamage>(dmg);
            }
            continue;
        }
    }
}

void FallDeathSystem::RespawnPlayer(Entity entity)
{
    TransformComponent* transform = mWorld->GetComponent<TransformComponent>(entity);
    MainPlayerComponent* player = mWorld->GetComponent<MainPlayerComponent>(entity);
    if (!transform || !player)
        return;

    Vec3 pos = player->mSpawnPosition;

    // 스폰 XZ 지점의 실제 지형 높이를 질의해 Y 보정
    if (auto& pw = mWorld->GetPhysicsWorld())
    {
        float ground = 0.0f;
        if (pw->TryQueryTerrainHeightNear(pos, pos.y,
                /*maxStepUp=*/2000.0f, /*maxDropDown=*/5000.0f, ground))
            pos.y = ground;
    }

    transform->mLocalPosition = pos;
    transform->mWorldMatrix = Matrix::CreateTranslation(pos);
    transform->mMovingVector = Vec3::Zero;

    // 중력 상태 초기화 (스폰 직후 낙하 방지)
    if (GravityComponent* grav = mWorld->GetComponent<GravityComponent>(entity))
    {
        grav->mHight = pos.y;
        grav->mGround = pos.y;
        grav->mGravity = 0.0f;
        grav->mFalling = false;
        grav->mDropping = false;
        grav->mGroundGraceLeft = 0.0f;
    }
    player->mFalling = false;

    // 이동 입력 잔여 초기화
    if (PlayerMovementComponent* pmc = mWorld->GetComponent<PlayerMovementComponent>(entity))
        pmc->mMovingDirection = Vec3::Zero;

    // HP 회복 + HUD 갱신
    if (HealthComponent* health = mWorld->GetComponent<HealthComponent>(entity))
    {
        health->mCurrentHp = health->mMaxHp;

        if (auto eventManager = mWorld->GetEventManager())
        {
            EvHealthChanged evt{};
            evt.target = entity;
            evt.currentHp = health->mCurrentHp;
            evt.maxHp = health->mMaxHp;
            eventManager->Enqueue<EvHealthChanged>(evt);
        }
    }
}
