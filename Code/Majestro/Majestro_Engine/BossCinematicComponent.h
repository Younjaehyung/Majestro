#pragma once
#include "Component.h"
#include "CameraView.h"


class BossCinematicComponent : public Component<BossCinematicComponent>
{
public:

    std::vector<Cinematic::CameraKeyframe> mKeys;

    // 재생 상태
    bool  mPlaying = false;   // 재생 중
    bool  mDone    = false;   // 1회 완료
    float mElapsed = 0.f;     // 재생 경과 시간(초)

    bool  HasSequence() const { return !mKeys.empty(); }
    float Duration()   const { return mKeys.empty() ? 0.f : mKeys.back().seconds; }
};
