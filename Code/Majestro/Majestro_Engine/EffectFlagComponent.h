#pragma once
#include "Component.h"

enum class EffectFlag : uint32
{
    None     = 0,
    HitFlash = 1,
    Dissolve = 2,
    Phase2   = 3,
};

struct EffectFlagComponent : public Component<EffectFlagComponent>
{
    EffectFlag flag     = EffectFlag::None;
    float      param    = 0.f;   // 효과 강도 (쉐이더에서 읽음)
    float      duration = 0.f;   // 지속 시간 (0 = 수동 RemoveComponent로 제거)
    float      elapsed  = 0.f;   // 내부 타이머
};
