#pragma once
#include "pch.h"
#include "Component.h"
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
    None,

    AttackUp,
    ScoreBoost,
    MoveSpeedUp,
    BuffPowerUp,

    ScoreOverTime,
    ShieldOverTime,
    HealOverTime,
    //de buff
    ShieldDown,
    Silence
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
    Entity mSource{};
    bool mIsRhythmEffect = false;

};

class BuffComponent : public Component<BuffComponent>
{
public:
    BuffComponent() = default;

    BuffData* FindBuff(BuffType type, Entity source = {});
    const BuffData* FindBuff(BuffType type, Entity source = {}) const;
    BuffData* FindRhythmBuffFromSource(Entity source);
    const BuffData* FindRhythmBuffFromSource(Entity source) const;
    void RecalculateDerivedEffects();
    BuffData& AddOrRefresh(const BuffData& buff);
    bool RemoveBuff(BuffType type, Entity source = {}, bool rhythmEffectOnly = false);

    std::vector<BuffData> mBuffs;


    float mAttackMultiplier = 1.0f;
    float mMoveSpeedMultiplier = 1.0f;

    bool buffPowerUp = false;
};
