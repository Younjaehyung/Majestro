#pragma once
#include "pch.h"
#include "Component.h"
#include "TimeUtils.h"
#include <algorithm>

enum class EffectKind
{
    Buff,
    Debuff
};

enum class DurationPolicy
{
    UntilSignal,
    Timed,
};

enum class BuffExecutionType
{
    Persistent, // 존재하는 동안 계속 적용
    Periodic    // 특정 간격마다 이벤트 발생
};


enum class BuffType
{
    AttackUp,
    MoveSpeedUp,
    ShieldOverTime,
    HealOverTime,
    //de buff
    ShieldDown
};
struct BuffData
{
    EffectKind mKind = EffectKind::Buff;
    BuffType mType = BuffType::AttackUp;
    DurationPolicy mDurationPolicy = DurationPolicy::Timed; //종료 신호까지 영구 지속, 시간적용
    BuffExecutionType mExecutionType = BuffExecutionType::Persistent; // 수정사항: 지속형 / 틱형 구분

    float mEndTime = 0.0f;

    float mTickInterval = 0.0f;
    float mNextTriggerTime = 0.0f;

};

class BuffComponent : public Component<BuffComponent>
{
public:
    BuffComponent() = default;
    
    std::vector<BuffData> mBuffs;
};
