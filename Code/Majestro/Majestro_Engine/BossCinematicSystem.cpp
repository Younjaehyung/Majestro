#include "pch.h"
#include "BossCinematicSystem.h"
#include "BossCinematicComponent.h"
#include "CameraSystem.h"
#include "GameRuleComponent.h"

std::vector<std::type_index> BossCinematicSystem::After() const
{
    return { typeid(CameraSystem) };
}

void BossCinematicSystem::Update(float dt)
{
    if (!mWorld->HasComponentPool<BossCinematicComponent>())
        return;

    const Entity singleton = mWorld->GetSingletonEntity();
    BossCinematicComponent* seq = mWorld->GetComponent<BossCinematicComponent>(singleton);
    if (!seq)
        return;

    // 현재 Phase 조회
    uint8 curPhase = mPrevPhase;
    if (GameRuleComponent* rule = mWorld->GetComponent<GameRuleComponent>(singleton))
        curPhase = rule->mGamePhase;

    // 트리거
    const uint8 boss = static_cast<uint8>(WavePhaseType::Boss);
    const bool enteredBoss = (curPhase == boss) && (mPrevPhase != boss);
    mPrevPhase = curPhase;

    if (enteredBoss && !seq->mDone)
    {
        if (seq->HasSequence())
        {
            // 카메라 시퀀스가 준비
            seq->mPlaying = true;
            seq->mElapsed = 0.f;
        }
        else
        {
            // 시퀀스 미준비 상태
            seq->mDone = true;
        }
    }

    if (!seq->mPlaying)
        return;

    // 보스 페이즈를 벗어나면 즉시 종료
    if (curPhase != boss)
    {
        Stop(seq);
        return;
    }

    seq->mElapsed += dt;
    Cinematic::ApplyCameraSequence(mWorld, seq->mKeys, seq->mElapsed);

    // 마지막 키프레임 도달 시 종료
    if (seq->mElapsed >= seq->Duration())
        Stop(seq);
}

void BossCinematicSystem::Stop(BossCinematicComponent* seq)
{
    seq->mPlaying = false;
    seq->mDone    = true;
}
