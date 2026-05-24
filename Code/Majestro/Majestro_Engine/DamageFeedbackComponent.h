#pragma once
#include "Component.h"

class HitReactionComponent : public Component<HitReactionComponent>
{
public:
    HitReactionComponent() = default;

    float mTimer{ 0.f };
    float mDuration{ 0.25f };

    // 펄스 피크 각도(라디안)
    // pitch: 양수 = 앞으로 끄덕임. 피격 시에는 보통 음수(뒤로 젖힘)
    // yaw  : 양수 = 좌측으로 비틂. 좌측 피격이면 음수(우측으로 젖힘)
    float mPeakPitch{ 0.f };
    float mPeakYaw{ 0.f };

    float mCurrentPitch{ 0.f };
    float mCurrentYaw{ 0.f };
};

class HitFlashComponent : public Component<HitFlashComponent>
{
public:
    HitFlashComponent() = default;

    float mTimer{ 0.f };           // 남은 시간 (0 이하면 비활성)
    float mDuration{ 0.2f };       // 플래시 총 길이
    float mPeakStrength{ 0.7f };   // 최대 틴트 강도 (0~1, forward_plus PS에서 red lerp 가중치)

    float mCurrentStrength{ 0.f };
};
