#include "pch.h"
#include "RhythmSystem.h"
#include "RhythmEmissiveComponent.h"
#include "World.h"

RhythmSystem::RhythmSystem(World* world): System(world)
{
    mPhase = SysPhase::Sim;
}

void RhythmSystem::Update(float deltaTime)
{
    UpdateEmissives(deltaTime);
}

void RhythmSystem::UpdateEmissives(float deltaTime)
{
    for (Entity e : mWorld->View<RhythmEmissiveComponent>())
    {
        auto* re = mWorld->GetComponent<RhythmEmissiveComponent>(e);
        if (!re) continue;

        if (re->mTimer > 0.f)
        {
            re->mTimer -= deltaTime;
            if (re->mTimer < 0.f)
                re->mTimer = 0.f;
        }

        const float target = (re->mTimer > 0.f) ? 1.f : 0.f;
        if (re->mCurrentGate < target)
        {
            const float rate = (re->mFadeIn > 0.f) ? (deltaTime / re->mFadeIn) : 1.f;
            re->mCurrentGate = (re->mCurrentGate + rate < target) ? re->mCurrentGate + rate : target;
        }
        else if (re->mCurrentGate > target)
        {
            const float rate = (re->mFadeOut > 0.f) ? (deltaTime / re->mFadeOut) : 1.f;
            re->mCurrentGate = (re->mCurrentGate - rate > target) ? re->mCurrentGate - rate : target;
        }
    }
}
