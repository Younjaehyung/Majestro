#include "pch.h"
#include "RhythmEmissiveSystem.h"

#include "RhythmEmissiveComponent.h"
#include "World.h"


RhythmEmissiveSystem::RhythmEmissiveSystem(World* world) : System(world)
{
    mPhase = SysPhase::Sim;
}

void RhythmEmissiveSystem::Update(float deltaTime)
{
    UpdateEmissives(deltaTime);
}

void RhythmEmissiveSystem::UpdateEmissives(float deltaTime)
{
    for (Entity entity : mWorld->View<RhythmEmissiveComponent>())
    {
        RhythmEmissiveComponent* emissive =
            mWorld->GetComponent<RhythmEmissiveComponent>(entity);
        if (!emissive)
            continue;

        if (emissive->mTimer > 0.0f)
        {
            emissive->mTimer -= deltaTime;
            emissive->mTimer = (std::max)(0.0f, emissive->mTimer);
        }

        const float targetGate = emissive->mTimer > 0.0f ? 1.0f : 0.0f;
        if (emissive->mCurrentGate < targetGate)
        {
            const float rate = emissive->mFadeIn > 0.0f
                ? deltaTime / emissive->mFadeIn
                : 1.0f;
            emissive->mCurrentGate = (std::min)(
                targetGate, emissive->mCurrentGate + rate);
        }
        else if (emissive->mCurrentGate > targetGate)
        {
            const float rate = emissive->mFadeOut > 0.0f
                ? deltaTime / emissive->mFadeOut
                : 1.0f;
            emissive->mCurrentGate = (std::max)(
                targetGate, emissive->mCurrentGate - rate);
        }
    }
}
