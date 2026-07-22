#include "pch.h"
#include "HighlightSystem.h"

#include "HighlightComponent.h"
#include "TransformComponent.h"
#include "VfxComponent.h"
#include "VfxSystem.h"
#include "SystemManager.h"
#include "World.h"

#include "DecalFactory.h"   
#include "CameraComponent.h"
#include "TagComponent.h"   
#include "Engine.h"         
#include "AudioManager.h"

#include <cmath>


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

        hl->mElapsed += deltaTime;

        // 남은 지속시간 감쇠
        if (hl->mRemainSec > 0.0f)
        {
            hl->mRemainSec -= deltaTime;
            hl->mRemainSec = (std::max)(0.0f, hl->mRemainSec);
        }


        if (hl->mRemainSec > 0.0f && !hl->mSparkFired)
        {
            FireBurst(entity, *hl);
            hl->mSparkFired = true;
        }


        const float targetGate = hl->mRemainSec > 0.0f ? 1.0f : 0.0f;
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

    // 로컬 적용
    if (IsLocalCaster(target))
    {
        CameraPunch(hl);
        if (hl.mBurstAudioEvent != nullptr)
            AUDIOMANAGER.PlayOneShot(hl.mBurstAudioEvent);
    }
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

void HighlightSystem::CameraPunch(const HighlightComponent& hl)
{
    // 카메라 연출
    for (Entity cameraEntity : mWorld->GetEntitiesWithComponent<CameraTypeComponent>())
    {
        CameraTypeComponent* camera = mWorld->GetComponent<CameraTypeComponent>(cameraEntity);
        if (camera == nullptr)
            continue;

        camera->TriggerShake(hl.mCamShakeAngles, hl.mCamShakeDuration, hl.mCamShakeFrequency);
        camera->TriggerDolly(hl.mCamDollyDistance, hl.mCamDollyHold, hl.mCamDollyInSpeed, hl.mCamDollyOutSpeed);
    }
}

bool HighlightSystem::IsLocalCaster(Entity target) const
{
    return mWorld->GetComponent<LocalPlayerComponent>(target) != nullptr;
}
