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

    bool  HasSequence() const { return !mKeys.empty(); }
    float Duration()    const { return mKeys.empty() ? 0.f : mKeys.back().seconds; }

    // 재생 시작. 목적지 씬 전환은 재생을 시작시킨 쪽(NetRecvSystem)이 관리한다.
    void Arm()
    {
        mElapsed = 0.f;
        mPlaying = true;
    }
};
