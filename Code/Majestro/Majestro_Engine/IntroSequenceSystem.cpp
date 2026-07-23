#include "pch.h"
#include "IntroSequenceSystem.h"
#include "IntroSequenceComponent.h"
#include "CameraComponent.h"
#include "TagComponent.h"        // MainCameraComponent
#include "TransformComponent.h"
#include "CameraSystem.h"
#include "GameRuleComponent.h"

std::vector<std::type_index> IntroSequenceSystem::After() const
{
    return { typeid(CameraSystem) };
}

void IntroSequenceSystem::Update(float dt)
{
    if (!mWorld->HasComponentPool<IntroSequenceComponent>())
        return;

    const Entity singleton = mWorld->GetSingletonEntity();
    IntroSequenceComponent* seq = mWorld->GetComponent<IntroSequenceComponent>(singleton);
    if (!seq)
        return;

    // ---- 현재 Phase 조회 (서버 권위) ----
    uint8 curPhase = mPrevPhase;
    if (GameRuleComponent* rule = mWorld->GetComponent<GameRuleComponent>(singleton))
        curPhase = rule->mGamePhase;

    // ---- 트리거: PreparePhase 진입 에지에서 1회 재생 시작 ----
    const uint8 prepare = static_cast<uint8>(WavePhaseType::Prepare);
    const bool enteredPrepare = (curPhase == prepare) && (mPrevPhase != prepare);
    mPrevPhase = curPhase;

    if (enteredPrepare && !seq->mDone && seq->HasSequence())
    {
        seq->mPlaying = true;
        seq->mElapsed = 0.f;
    }

    if (!seq->mPlaying)
        return;

    // ---- Prepare 단계를 벗어나면 즉시 종료(카메라/입력 즉시 복귀) ----
    if (curPhase != prepare)
    {
        Stop(seq);
        return;
    }

    Apply(seq, dt);

    // 마지막 키프레임 도달 시 종료
    if (seq->mElapsed >= seq->Duration())
        Stop(seq);
}

void IntroSequenceSystem::Apply(IntroSequenceComponent* seq, float dt)
{
    seq->mElapsed += dt;

    Cinematic::ApplyCameraSequence(mWorld, seq->mKeys, seq->mElapsed);
}

void IntroSequenceSystem::Stop(IntroSequenceComponent* seq)
{
    seq->mPlaying = false;
    seq->mDone    = true;
}
