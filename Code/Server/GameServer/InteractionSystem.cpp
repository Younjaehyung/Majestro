#include "pch.h"
#include "InteractionSystem.h"

#include "World.h"
#include "EventManager.h"
#include "GameEvents.h"
#include "GameTimer.h"

#include "InteractableComponent.h"
#include "TransformComponent.h"
#include "ColliderComponent.h"
#include "PhysicsWorld.h"

#include "PlayerComponent.h"
#include "EnemyComponent.h"
#include "HealthComponent.h"

InteractionSystem::InteractionSystem(World* world)
    : System(world)
{
    mPhase = SysPhase::Sim;
}



bool InteractionSystem::MatchesTargetMask(World* world, Entity e, uint8 mask)
{
    if (mask & InteractableTarget_All) return true;

    if ((mask & InteractableTarget_Player) && world->HasComponent<MainPlayerComponent>(e))
        return true;

    if ((mask & InteractableTarget_Enemy) && world->HasComponent<EnemyComponent>(e))
        return true;

    return false;
}

void InteractionSystem::Update(float dt)
{
    (void)dt;

    if (!mWorld->HasComponentPool<InteractableComponent>()) return;
    if (!mWorld->HasComponentPool<BoxColliderComponent>())  return;
    if (!mWorld->HasComponentPool<TransformComponent>())    return;

    auto eventManager = mWorld->GetEventManager();
    if (!eventManager) return;

    const float now = GetServerTotalTimeSeconds();

    candidates.clear();
	

    if (mWorld->HasComponentPool<MainPlayerComponent>())
    {
        for (auto e : mWorld->GetEntitiesWithComponents<MainPlayerComponent, TransformComponent>()) {
            candidates.push_back(e);
           
        }
    }
    if (mWorld->HasComponentPool<EnemyComponent>())
    {
        for (auto e : mWorld->GetEntitiesWithComponents<EnemyComponent, TransformComponent>()) {
            candidates.push_back(e);
            
        }
    }
    if (candidates.empty()) return;


    auto triggers = mWorld->GetEntitiesWithComponents<InteractableComponent, TransformComponent>();

    for (Entity t : triggers)
    {
        auto* inter = mWorld->GetComponent<InteractableComponent>(t);
        auto* trTr = mWorld->GetComponent<TransformComponent>(t);
  
        if (!inter || !trTr ) continue;
        if (!inter->mActive) continue;
        if (now < inter->mNextAvailableTime) continue;



        for (Entity user : candidates)
        {
            if (!MatchesTargetMask(mWorld, user, inter->mTargetMask)) continue;


            if (HealthComponent* hp = mWorld->GetComponent<HealthComponent>(user))
            {
                // 사망시 넘어감
                if (hp->IsDead()) continue;
            }

            TransformComponent* userts = mWorld->GetComponent<TransformComponent>(user);
            const Vec3 triggerPos = userts->mLocalPosition;

            bool hit = false;
            if (inter->mShape == InteractableShape::Box) {
                auto* trCol = mWorld->GetComponent<BoxColliderComponent>(t);
                if (!trCol) continue;
                PhysicsWorld::UpdateWorldOBB(trTr, trCol);
                auto* userCol = mWorld->GetComponent<BoxColliderComponent>(user);
                if (!userCol) continue;
                hit = trCol->mWorldOBB.Intersects(userCol->mWorldOBB);
            }
            else { // Sphere
                Vec3 d = trTr->mLocalPosition - triggerPos;
                if (inter->mIgnoreY) d.y = 0.f;
                hit = (d.LengthSquared() <= inter->mRadius * inter->mRadius);
            }
            if (!hit) continue;



            switch (inter->mKind)
            {
            case InteractableKind::HealPack:    ApplyHealPack(user, t, *inter);  break;
            case InteractableKind::ArmorPack:   ApplyArmorPack(user, t, *inter); break;
            case InteractableKind::JumpPad:     ApplyJumpPad(user, t, *inter);   break;
            case InteractableKind::SpeedPad:    ApplySpeedPad(user, t, *inter);  break;
            case InteractableKind::DamageZone:  ApplyDamageZone(user, t, *inter); break;
            case InteractableKind::Checkpoint:
            case InteractableKind::None:
            default: break;
            }


            inter->mLastUserId = user.GetID();
            inter->mLastTriggerTime = now;
            inter->mNextAvailableTime = now + inter->mCooldown;

            if (inter->mOneShot)
            {   // 일회용
                inter->mActive = false;
                eventManager->Enqueue<EvInteractableConsumed>(EvInteractableConsumed{ t, user });
                break;
            }

            // 쿨타임있는거면 스킵
            if (inter->mCooldown > 0.0f) break;
        }
    }
}
void InteractionSystem::ApplyHealPack(Entity user, Entity trigger, const InteractableComponent& i)
{
    EvHeal heal{};
    heal.target     = user;
    heal.amount     = static_cast<int32>(i.mValueA);
    heal.instigator = trigger;
    mWorld->GetEventManager()->Enqueue<EvHeal>(heal);
}

void InteractionSystem::ApplyArmorPack(Entity user, Entity trigger, const InteractableComponent& i)
{
    // 방어력 즉시 회복 전용 이벤트가 아직 없으므로 EvArmorChanged로 대체:
    // ArmorComponent 직접 조정 후 변경 이벤트 송출. (Damage/Heal과 대칭 구조가 필요하면 추후 EvArmorHeal 분리)
    (void)trigger;
    // 현재 리팩터 범위를 넘어 직접 접근은 지양. 최소 구현으로 target만 두고 추후 확장.
    // -> ArmorComponent 포인터를 직접 조정하는 대신, DamageSystem 경유와 유사한 경로가 필요하면
    //    EvArmorHeal 이벤트를 추가해 DamageSystem 또는 별도 시스템이 소비하게 하자.
    // 지금은 placeholder로 EvHeal 발행(힐팩과 동일 처리)해두지 않고 no-op.
    (void)user; (void)i;
}

// 점프대
void InteractionSystem::ApplyJumpPad(Entity user, Entity trigger, const InteractableComponent& i)
{
    (void)trigger;

    EvImpulse impulse{};
    impulse.target = user;
    impulse.x = 0.0f;
    impulse.y = i.mValueA;
    impulse.z = 0.0f;
    impulse.source = ImpulseSource::JumpPad;
    mWorld->GetEventManager()->Enqueue<EvImpulse>(impulse);
}

void InteractionSystem::ApplySpeedPad(Entity user, Entity trigger, const InteractableComponent& i)
{
    (void)trigger;

    // 속도 부스트: BuffSystem에게 위임
    EvBuffRequest req{};
    req.target = user;
    req.skillType = i.mBuffType; // 호출자가 적절한 SkillType을 지정하도록 약속
    mWorld->GetEventManager()->Enqueue<EvBuffRequest>(req);
}

void InteractionSystem::ApplyDamageZone(Entity user, Entity trigger, InteractableComponent& i)
{
    // 지속 데미지 영역: 틱 간격을 Interactable 자체에서 관리 (cooldown 재사용).
    // 호출자가 mCooldown = mValueB(틱 간격)으로 설정해두면 자동으로 동작.
    EvDamage dmg{};
    dmg.target     = user;
    dmg.amount     = static_cast<int32>(i.mValueA);
    dmg.instigator = trigger;
    mWorld->GetEventManager()->Enqueue<EvDamage>(dmg);
}
