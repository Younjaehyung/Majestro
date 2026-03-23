#pragma once
#include "System.h"
#include "EffectFlagComponent.h"
#include "World.h"

// EffectFlagComponent 타이머를 관리하는 시스템
// duration > 0인 컴포넌트를 시간 경과 후 자동 제거
// Phase: Pre (렌더 이전에 상태 정리)
class EffectFlagSystem : public System
{
public:
    EffectFlagSystem(World* world) : System(world)
    {
        mPhase = SysPhase::Pre;
        mOrder = 0;
    }

    void Initialize() override {}

    void Update(float dt) override
    {
        if (!mWorld->HasComponentPool<EffectFlagComponent>())
            return;

        // 만료된 컴포넌트 수집 후 일괄 제거 (순회 중 제거 방지)
        std::vector<Entity> toRemove;

        for (auto entity : mWorld->View<EffectFlagComponent>())
        {
            auto* efx = mWorld->GetComponent<EffectFlagComponent>(entity);
            if (efx->duration <= 0.f)
                continue; 

            efx->elapsed += dt;
            if (efx->elapsed >= efx->duration)
                toRemove.push_back(entity);
        }

        for (auto entity : toRemove)
            mWorld->RemoveComponent<EffectFlagComponent>(entity);
    }
};
