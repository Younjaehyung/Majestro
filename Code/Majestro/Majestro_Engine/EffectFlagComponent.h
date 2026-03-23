#pragma once
#include "Component.h"

enum class EffectFlag : uint32
{
    None     = 0,
	Infinite, // 지속 시간 무한 (duration 무시)
    HitFlash,
    Dissolve,
    Phase2  ,
};

class EffectFlagComponent : public Component<EffectFlagComponent>
{
public:
    EffectFlag mFlag     = EffectFlag::None;
    float      mParam    = 0.f;   // 효과 강도 (쉐이더에서 읽음)
    float      mDuration = 0.f;   // 지속 시간 (0 = 수동 RemoveComponent로 제거)
    float      mElapsed  = 0.f;   // 내부 타이머
};
