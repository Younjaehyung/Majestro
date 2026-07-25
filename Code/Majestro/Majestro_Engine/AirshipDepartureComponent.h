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

    // 재생 완료 후 서버에 요청할 목적지 씬
    SceneId mTargetScene = SceneId::FirstGame;

    bool  HasSequence() const { return !mKeys.empty(); }
    float Duration()    const { return mKeys.empty() ? 0.f : mKeys.back().seconds; }

    // 연출 예약 + 재생 시작.
    void Arm(SceneId target)
    {
        mTargetScene = target;
        mElapsed     = 0.f;
        mPlaying     = true;
    }
};
