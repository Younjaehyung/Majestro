#pragma once
#include "pch.h"
#include "Component.h"
#include "CameraView.h"


class AirshipDepartureComponent : public Component<AirshipDepartureComponent>
{
public:
    std::vector<Cinematic::CameraKeyframe> mKeys;

    // 재생 상태
    bool  mPlaying = false;   // 재생 중 — 입력 잠금
    float mElapsed = 0.f;     // 재생 경과 시간(초)

    // 재생 완료 시 실행할 씬 전환 정보
    SceneId mTargetScene    = SceneId::FirstGame;
    wstring mLoadingMessage;
    bool    mNeedsGameStart = false;   // 완료 시 RequestPendingGameStart 필요 여부

    bool  HasSequence() const { return !mKeys.empty(); }
    float Duration()    const { return mKeys.empty() ? 0.f : mKeys.back().seconds; }

    // 연출 예약 + 재생 시작
    void Arm(SceneId target, const wstring& message, bool needsGameStart)
    {
        mTargetScene    = target;
        mLoadingMessage = message;
        mNeedsGameStart = needsGameStart;
        mElapsed        = 0.f;
        mPlaying        = true;
    }
};
