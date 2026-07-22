#include "pch.h"
#include "HighlightSystem.h"

#include "HighlightComponent.h"
#include "PlayerComponent.h"
#include "TransformComponent.h"
#include "VfxComponent.h"
#include "VfxSystem.h"
#include "SystemManager.h"
#include "World.h"

#include "DecalFactory.h"
#include "TagComponent.h"
#include "Engine.h"


namespace
{
    bool IsUltimateState(int upperState)
    {
        return upperState == static_cast<int>(ReplicatedActionState::UltimateIntro)
            || upperState == static_cast<int>(ReplicatedActionState::Special);
    }
}


HighlightSystem::HighlightSystem(World* world) : System(world)
{
    mPhase = SysPhase::Sim;
}

void HighlightSystem::Update(float deltaTime)
{
    UpdateHighlights(deltaTime);
}

void HighlightSystem::UpdateHighlights(float deltaTime)
{

    for (Entity entity : mWorld->View<HighlightComponent>())
    {
        HighlightComponent* hl = mWorld->GetComponent<HighlightComponent>(entity);
        if (!hl)
            continue;

        bool ultimateActive = false;
        if (MainPlayerComponent* player = mWorld->GetComponent<MainPlayerComponent>(entity))
            ultimateActive = IsUltimateState(player->mUpperState);

        hl->mElapsed += deltaTime;

        // 번쩍임 + 스파크
        if (ultimateActive)
        {
            if (!hl->mSparkFired)
            {
                hl->mFadeGate   = hl->mBurstGate;
                FireBurst(entity, *hl);
                hl->mSparkFired = true;
            }
        }
        else
        {
            hl->mSparkFired = false;
        }

        // 페이드 게이트 목표
        const float targetGate = ultimateActive ? 1.0f : 0.0f;

        if (hl->mFadeGate < targetGate)
        {
            const float rate = hl->mFadeIn > 0.0f ? deltaTime / hl->mFadeIn : 1.0f;
            hl->mFadeGate = (std::min)(targetGate, hl->mFadeGate + rate);
        }
        else if (hl->mFadeGate > targetGate)
        {
            const float rate = hl->mFadeOut > 0.0f ? deltaTime / hl->mFadeOut : 1.0f;
            hl->mFadeGate = (std::max)(targetGate, hl->mFadeGate - rate);
        }


        const float wave  = 0.5f + 0.5f * std::sin(hl->mElapsed * hl->mPulseSpeed);
        const float pulse = 1.0f - hl->mPulseDepth * wave;

        hl->mCurrentIntensity = hl->mBaseIntensity * hl->mFadeGate * pulse;
    }
}

void HighlightSystem::FireBurst(Entity target, const HighlightComponent& hl)
{

    SpawnSpark(target, hl);        // 상승 오라/기둥
    SpawnGroundShock(target, hl);  // 지면 충격파 데칼

}

void HighlightSystem::SpawnSpark(Entity target, const HighlightComponent& hl)
{
    if (hl.mSparkVfx == nullptr)
        return;

    TransformComponent* tr = mWorld->GetComponent<TransformComponent>(target);
    if (tr == nullptr)
        return;

    VfxSystem* vfxSystem = mWorld->GetSystemManager()->GetSystem<VfxSystem>();
    if (vfxSystem == nullptr)
        return;

    // 궁극기 VFX
    const Entity fx = vfxSystem->PlayOneShot(
        hl.mSparkVfx, tr->mWorldPosition + hl.mSparkOffset, Vec3::Zero, hl.mSparkScale);

    if (!fx.IsValid())
        return;

    if (VfxComponent* vc = mWorld->GetComponent<VfxComponent>(fx))
    {
        vc->mFollowTarget = target;          
        vc->mFollowOffset = hl.mSparkOffset;
    }
}

void HighlightSystem::SpawnGroundShock(Entity target, const HighlightComponent& hl)
{
    if (hl.mGroundDecalRadius <= 0.0f)
        return;

    TransformComponent* tr = mWorld->GetComponent<TransformComponent>(target);
    if (tr == nullptr)
        return;

    // 캐릭터 발밑에 충격파 데칼
    DecalFactory::StampGroundCrack(
        mWorld,
        tr->mWorldPosition,
        hl.mGroundDecalRadius,
        hl.mGroundDecalTex != nullptr ? hl.mGroundDecalTex : L"",
        hl.mGroundDecalColor,
        hl.mGroundDecalLifetime,
        hl.mGroundDecalCols,
        hl.mGroundDecalRows,
        hl.mGroundDecalIndex);
}
