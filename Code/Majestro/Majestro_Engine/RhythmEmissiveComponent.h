#pragma once
#include "Component.h"

class RhythmEmissiveComponent : public Component<RhythmEmissiveComponent>
{
public:
    RhythmEmissiveComponent() = default;

    float mTimer{ 0.f };       // 남은 발광 시간 (0 이하면 페이드아웃 시작)
    float mDuration{ 3.f };    // 리듬 변경 시 켜지는 시간

    float mFadeIn{ 0.3f };     // 페이드인 시간
    float mFadeOut{ 0.5f };    // 페이드아웃 시간
    float mCurrentGate{ 0.f }; // 쉐이더로 전달되는 게이트 (0~1, Extra.z)
};
