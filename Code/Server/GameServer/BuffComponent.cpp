#include "pch.h"
#include "BuffComponent.h"

BuffData* BuffComponent::FindBuff(BuffType type)
{
    auto it = std::find_if(mBuffs.begin(), mBuffs.end(), [type](const BuffData& buff)
    {
        return buff.mType == type;
    });
    return (it == mBuffs.end()) ? nullptr : &(*it);
}

void BuffComponent::ApplyBuffEffect(const BuffData& buff)
{
    switch (buff.mType)
    {
    case BuffType::BuffPowerUp:
        buffPowerUp = true;
        if(FindBuff(BuffType::AttackUp))  mAttackMultiplier = 2.0f;
        else if(FindBuff(BuffType::ScoreBoost))  mAttackMultiplier = 4.0f;
        else if(FindBuff(BuffType::MoveSpeedUp))  mMoveSpeedMultiplier = 1.3f;
        break;

    case BuffType::AttackUp:
        if(buffPowerUp) mAttackMultiplier = 2.0f;
        else mAttackMultiplier = 1.5f;
        break;

    case BuffType::MoveSpeedUp:
        if (buffPowerUp)mMoveSpeedMultiplier = 1.1f;
        else mMoveSpeedMultiplier = 1.3f;
        break;

    default:
        break;
    }
}

void BuffComponent::RemoveBuffEffect(const BuffData& buff)
{
    switch (buff.mType)
    {
    case BuffType::BuffPowerUp:
        buffPowerUp = false;
        break;

    case BuffType::AttackUp:
        mAttackMultiplier = 1.0f;
        break;

    case BuffType::MoveSpeedUp:
        mMoveSpeedMultiplier = 1.0f;
        break;

    default:
        break;
    }
}

BuffData& BuffComponent::AddOrRefresh(const BuffData& buff)
{

    if (BuffData* existing = FindBuff(buff.mType))
    {
        RemoveBuffEffect(*existing);

        *existing = buff;

        ApplyBuffEffect(*existing);
        return *existing;
    }


    mBuffs.push_back(buff);
    ApplyBuffEffect(mBuffs.back());
    return mBuffs.back();
}

bool BuffComponent::RemoveBuff(BuffType type)
{
    auto it = std::find_if(mBuffs.begin(), mBuffs.end(), [type](const BuffData& buff)
        {
            return buff.mType == type;
        });

    if (it == mBuffs.end())
        return false;

    RemoveBuffEffect(*it);
    mBuffs.erase(it);
    return true;
}