#include "pch.h"
#include "BossTileGlowSystem.h"

#include "BossTileGlowComponent.h"
#include "EventManager.h"
#include "GameEvents.h"
#include "TransformComponent.h"
#include "World.h"

namespace
{
   
    constexpr float kTileVfxHeightOffset = 5.0f;

    // 예고 시간 맥동
    float PulseSpeedForDuration(float durationSec)
    {
        if (durationSec <= 0.0f)
            return 6.0f;

        // 예고 구간 동안 대략 8회 점멸
        return (std::max)(3.0f, 8.0f * 3.14159265f / durationSec);
    }
}

BossTileGlowSystem::BossTileGlowSystem(World* world) : System(world)
{
    mPhase = SysPhase::Sim;
}

void BossTileGlowSystem::Update(float deltaTime)
{
    ConsumeTileEvents();
    UpdateGlows(deltaTime);
}

void BossTileGlowSystem::ConsumeTileEvents()
{
    auto eventManager = mWorld->GetEventManager();
    if (eventManager == nullptr)
        return;

    eventManager->Consume<EvBossTileUpdate>(
        [this, &eventManager](const EvBossTileUpdate& event)
        {
            const BossTilePhase phase = static_cast<BossTilePhase>(event.phase);
            const float pulseSpeed = PulseSpeedForDuration(event.durationSec);

            for (Entity entity : mWorld->View<BossTileGlowComponent>())
            {
                BossTileGlowComponent* glow =
                    mWorld->GetComponent<BossTileGlowComponent>(entity);
                if (!glow)
                    continue;

                if (phase == BossTilePhase::Clear)
                {
                    glow->mTargetIntensity = 0.0f;
                    glow->mPulsing = false;
                    continue;
                }

                TransformComponent* transform =
                    mWorld->GetComponent<TransformComponent>(entity);
                if (!transform || event.tileSize <= 0.0f)
                    continue;

              
                const Vec3 tilePosition = transform->mWorldMatrix.Translation();

                const int column = static_cast<int>(std::lround(
                    (tilePosition.x - event.originX) / event.tileSize));
                const int row = static_cast<int>(std::lround(
                    (tilePosition.z - event.originZ) / event.tileSize));


                const bool onArenaFloor =
                    std::fabs(tilePosition.y - event.originY) <= event.tileSize * 0.5f;

                const bool inGrid = onArenaFloor &&
                    column >= 0 && column < static_cast<int>(event.columnCount) &&
                    row >= 0 && row < static_cast<int>(event.rowCount);
                const bool isActiveTile =
                    inGrid && ((column + row) & 1) == static_cast<int>(event.parity);

                if (!isActiveTile)
                {

                    glow->mTargetIntensity = 0.0f;
                    glow->mPulsing = false;
                    continue;
                }

                if (phase == BossTilePhase::Warn)
                {
                    glow->mColor = glow->mWarnColor;
                    glow->mTargetIntensity = 1.0f;
                    glow->mPulsing = true;
                    glow->mPulseSpeed = pulseSpeed;
                    glow->mElapsed = 0.0f;

                    eventManager->Enqueue(EvVfxSpawnRequest{
                        event.skillType,
                        static_cast<uint8>(EffectSpawnReason::Fire),
                        Vec3(tilePosition.x,
                             event.originY + kTileVfxHeightOffset,
                             tilePosition.z),
                        Vec3::Zero,
                        0 });
                }
                else // Explode
                {
                    
                    glow->mColor = glow->mExplodeColor;
                    glow->mCurrentIntensity = 1.0f;
                    glow->mTargetIntensity = 0.0f;
                    glow->mPulsing = false;
                }
            }
        });
}

void BossTileGlowSystem::UpdateGlows(float deltaTime)
{
    for (Entity entity : mWorld->View<BossTileGlowComponent>())
    {
        BossTileGlowComponent* glow = mWorld->GetComponent<BossTileGlowComponent>(entity);
        if (!glow)
            continue;

        if (glow->mCurrentIntensity <= 0.0f && glow->mTargetIntensity <= 0.0f)
        {
            glow->mRenderIntensity = 0.0f;
            continue;
        }

        glow->mElapsed += deltaTime;

        if (glow->mCurrentIntensity < glow->mTargetIntensity)
        {
            const float rate = glow->mFadeIn > 0.0f ? deltaTime / glow->mFadeIn : 1.0f;
            glow->mCurrentIntensity =
                (std::min)(glow->mTargetIntensity, glow->mCurrentIntensity + rate);
        }
        else if (glow->mCurrentIntensity > glow->mTargetIntensity)
        {
            const float rate = glow->mFadeOut > 0.0f ? deltaTime / glow->mFadeOut : 1.0f;
            glow->mCurrentIntensity =
                (std::max)(glow->mTargetIntensity, glow->mCurrentIntensity - rate);
        }

        float pulse = 1.0f;
        if (glow->mPulsing)
        {
            // 0을 완전히 찍지 않도록 깊이만큼만 흔든다.
            pulse = 1.0f - glow->mPulseDepth *
                (0.5f - 0.5f * std::cos(glow->mElapsed * glow->mPulseSpeed));
        }

        glow->mRenderIntensity = glow->mCurrentIntensity * pulse;
    }
}
