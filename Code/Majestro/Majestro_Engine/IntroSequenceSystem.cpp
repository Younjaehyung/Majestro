#include "pch.h"
#include "IntroSequenceSystem.h"
#include "IntroSequenceComponent.h"
#include "CameraComponent.h"
#include "TagComponent.h"        // MainCameraComponent
#include "TransformComponent.h"
#include "CameraSystem.h"
#include "GameRuleComponent.h"
#include "CameraView.h"
#include "EventManager.h"
#include "GameEvents.h"

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

    if (enteredPrepare && !seq->mDone)
    {
        if (seq->HasSequence())
        {
            seq->mPlaying = true;
            seq->mElapsed = 0.f;
        }
        else
        {
            // 시퀀스 미준비 씬은 재생 없이 즉시 완료 처리한다.
            // 이렇게 해야 아래 완료 보고가 나가고 서버가 이 클라를 계속 기다리지 않는다.
            seq->mDone = true;
        }
    }

    if (!seq->mPlaying)
    {
        TryReportFinished(seq);
        return;
    }

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

// 시퀀스가 끝나고 뒤따르는 컷인까지 모두 내려간 뒤에 서버로 완료를 보고한다.
// 컷인 여부는 Cinematic::IsAnyCinematicPlaying 이 판정하므로, 나중에 인트로 전용 컷인을
// 추가하더라도 그 컴포넌트를 IsAnyCinematicPlaying 에 등록하기만 하면 여기는 그대로 동작한다.
void IntroSequenceSystem::TryReportFinished(const IntroSequenceComponent* seq)
{
    if (mReported || !seq->mDone)
        return;

    // mDone 이 켜진 프레임에는 컷인이 아직 자신을 켜지 못했을 수 있어 한 프레임 미룬다.
    if (!mDoneSeen)
    {
        mDoneSeen = true;
        return;
    }

    if (Cinematic::IsAnyCinematicPlaying(mWorld))
        return;

    mWorld->GetEventManager()->Enqueue<EvIntroFinished>(EvIntroFinished{});
    mReported = true;
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
