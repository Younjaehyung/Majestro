#include "BuffComponent.h"

BuffData* BuffComponent::FindBuff(BuffType type)
{
    auto it = std::find_if(mBuffs.begin(), mBuffs.end(), [type](const BuffData& buff)
    {
        return buff.mType == type;
    });
    return (it == mBuffs.end()) ? nullptr : &(*it);
}

BuffData& BuffComponent::AddOrRefresh(const BuffData& buff)
{

    if (BuffData* existing = FindBuff(buff.mType))
    {
        *existing = buff;
        return *existing;
    }

    mBuffs.push_back(buff);
    return mBuffs.back();
}

bool BuffComponent::RemoveBuff(BuffType type)
{
    const auto previousEnd = std::remove_if(mBuffs.begin(), mBuffs.end(), [type](const BuffData& buff)
        {
            return buff.mType == type;
        });

    if (previousEnd == mBuffs.end())
    return false;

    mBuffs.erase(previousEnd, mBuffs.end());
    return true;
}