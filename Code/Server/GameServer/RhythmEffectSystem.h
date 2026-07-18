#pragma once

#include "System.h"


class MainPlayerComponent;
class RhythmStateComponent;
class RhythmEffectComponent;


class RhythmEffectSystem : public System
{
public:
    explicit RhythmEffectSystem(World* world);

    void Update(float deltaTime) override;
    void ApplyCurrentRhythmEffects(Entity playerEntity);

private:

	// 리듬 상태에 따른 변형 효과 적용
    void UpdateVariantModifiers(
        RhythmStateComponent& rhythmState,
        RhythmEffectComponent& rhythmEffect);

	// 리듬 효과에 따른 플레이어 리소스(체력, 방어막 등) 동기화
    void SyncVariantResourceStats(
        Entity playerEntity,
        RhythmEffectComponent& rhythmEffect);

	// 리듬 효과 제공자에 대한 동기화
    void SyncBaseRhythmEffectForProvider(
        Entity provider,
        MainPlayerComponent& providerPlayer,
        RhythmStateComponent& rhythmState,
        const RhythmEffectComponent& rhythmEffect);
};
