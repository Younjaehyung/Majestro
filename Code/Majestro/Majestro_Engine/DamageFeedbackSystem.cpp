#include "pch.h"
#include "DamageFeedbackSystem.h"
#include "DamageFeedbackComponent.h"
#include "TransformComponent.h"
#include "GameEvents.h"
#include "World.h"
#include "MathUtils.h"


DamageFeedbackSystem::DamageFeedbackSystem(World* world)
    : System(world)
{
    mPhase = SysPhase::Sim;
}

void DamageFeedbackSystem::Update(float deltaTime)
{
    // 피격 이벤트 소비 : HitFlash / HitReaction 컴포넌트 부착, 리셋
    mWorld->GetEventManager()->Consume<EvHealthChanged>([&](const EvHealthChanged& e)
    {
        const int32 delta = e.previousHp - e.hp;
        if (delta <= 0) return; // 힐/변동 없음 무시

        // Hit flash
        auto* flash = mWorld->GetComponent<HitFlashComponent>(e.target);
        if (!flash)
            flash = &mWorld->AddComponent<HitFlashComponent>(e.target);
        flash->mTimer = flash->mDuration;

        // Hit reaction (상체 움찔)
        auto* reaction = mWorld->GetComponent<HitReactionComponent>(e.target);
        if (!reaction)
            reaction = &mWorld->AddComponent<HitReactionComponent>(e.target);

        const TransformComponent* targetTr = mWorld->GetComponent<TransformComponent>(e.target);
        DecomposeHitDirection(e.hitDirection, targetTr, reaction->mPeakPitch, reaction->mPeakYaw);
        reaction->mTimer = reaction->mDuration;
    });


    {
        std::vector<Entity> expired;
        for (Entity e : mWorld->View<HitFlashComponent>())
        {
            auto* f = mWorld->GetComponent<HitFlashComponent>(e);
            if (!f) continue;

            f->mTimer -= deltaTime;
            if (f->mTimer <= 0.f)
            {
                expired.push_back(e);
                continue;
            }

            // 선형 감쇠
            const float t = f->mDuration > 0.f ? (f->mTimer / f->mDuration) : 0.f;
            f->mCurrentStrength = f->mPeakStrength * t;
        }

        for (Entity e : expired)
            mWorld->RemoveComponent<HitFlashComponent>(e);
    }


    {
        std::vector<Entity> expired;
        for (Entity e : mWorld->View<HitReactionComponent>())
        {
            auto* r = mWorld->GetComponent<HitReactionComponent>(e);
            if (!r) continue;

            r->mTimer -= deltaTime;
            if (r->mTimer <= 0.f)
            {
                expired.push_back(e);
                continue;
            }

           
            const float t = 1.f - (r->mTimer / r->mDuration);
            const float curve = std::sin(t * kPI);
            r->mCurrentPitch = r->mPeakPitch * curve;
            r->mCurrentYaw   = r->mPeakYaw   * curve;
        }

        for (Entity e : expired)
            mWorld->RemoveComponent<HitReactionComponent>(e);
    }
}

void DamageFeedbackSystem::DecomposeHitDirection(const Vec3& hitDirWorld,
    const TransformComponent* targetTr,
    float& outPitch, float& outYaw)
{
    const float lenSq = hitDirWorld.LengthSquared();
    if (lenSq < 1e-6f || targetTr == nullptr)
    {
		// 없을시 단순 끄덕임만 적용
        outPitch = -mHitReactionPeakPitch;
        outYaw = 0.f;
        return;
    }

    Vec3 dir = hitDirWorld;
    dir.Normalize();

    // 타깃의 yaw만 사용
    const float yawRad = targetTr->mLocalRotationE.y * (kPI / 180.f);
    const float c = std::cos(yawRad);
    const float s = std::sin(yawRad);

    const float dotForward = dir.x * s + dir.z * c;          // 양수면 뒤에서 맞음
    const float dotRight = dir.x * c + dir.z * (-s);         // 양수면 좌측에서 맞음

    // 충격은 반대 방향으로 상체를 민다.
    outPitch = -dotForward * mHitReactionPeakPitch;
    outYaw = -dotRight * mHitReactionPeakYaw;
}
