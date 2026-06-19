#include "pch.h"
#include "BuffComponent.h"

BuffData* BuffComponent::FindBuff(BuffType type, Entity source)
{
    return const_cast<BuffData*>(static_cast<const BuffComponent*>(this)->FindBuff(type, source));
}

const BuffData* BuffComponent::FindBuff(BuffType type, Entity source) const
{
    auto it = std::find_if(mBuffs.begin(), mBuffs.end(), [type, source](const BuffData& buff)
    {
        if (buff.mType != type)
            return false;
        return !source.IsValid() || buff.mSource == source;
    });
    return (it == mBuffs.end()) ? nullptr : &(*it);
}

BuffData* BuffComponent::FindRhythmBuffFromSource(Entity source)
{
    return const_cast<BuffData*>(static_cast<const BuffComponent*>(this)->FindRhythmBuffFromSource(source));
}

const BuffData* BuffComponent::FindRhythmBuffFromSource(Entity source) const
{
    auto it = std::find_if(mBuffs.begin(), mBuffs.end(), [source](const BuffData& buff)
    {
        return buff.mIsRhythmEffect && buff.mSource == source;
    });
    return (it == mBuffs.end()) ? nullptr : &(*it);
}

void BuffComponent::RecalculateDerivedEffects()
{
    const bool hasBuffPowerUp = FindBuff(BuffType::BuffPowerUp) != nullptr;
    const bool hasAttackUp = FindBuff(BuffType::AttackUp) != nullptr;
    const bool hasScoreBoost = FindBuff(BuffType::ScoreBoost) != nullptr;
    const bool hasMoveSpeedUp = FindBuff(BuffType::MoveSpeedUp) != nullptr;
    const bool hasMoveSpeedUp10 = FindBuff(BuffType::MoveSpeedUp10) != nullptr;

    buffPowerUp = hasBuffPowerUp;
    mAttackMultiplier = 1.0f;
    mMoveSpeedMultiplier = 1.0f;

    if (hasBuffPowerUp)
    {
        if (hasAttackUp)
            mAttackMultiplier = 2.0f;
        else if (hasScoreBoost)
            mAttackMultiplier = 4.0f;
    }
    else if (hasAttackUp)
    {
        mAttackMultiplier = 1.5f;
    }

    if (hasMoveSpeedUp)
        mMoveSpeedMultiplier = hasBuffPowerUp ? 1.1f : 1.3f;
    else if (hasMoveSpeedUp10)
        mMoveSpeedMultiplier = 1.1f;
}

BuffData& BuffComponent::AddOrRefresh(const BuffData& buff)
{
    if (BuffData* existing = FindBuff(buff.mType, buff.mSource))
    {
        *existing = buff;
        RecalculateDerivedEffects();
        return *existing;
    }

    mBuffs.push_back(buff);
    RecalculateDerivedEffects();
    return mBuffs.back();
}

bool BuffComponent::RemoveBuff(BuffType type, Entity source, bool rhythmEffectOnly)
{
    auto it = std::find_if(mBuffs.begin(), mBuffs.end(), [type, source, rhythmEffectOnly](const BuffData& buff)
        {
            if (buff.mType != type)
                return false;
            if (source.IsValid() && buff.mSource != source)
                return false;
            if (rhythmEffectOnly && !buff.mIsRhythmEffect)
                return false;
            return true;
        });

    if (it == mBuffs.end())
        return false;

    mBuffs.erase(it);
    RecalculateDerivedEffects();
    return true;
}
