#pragma once
#include "pch.h"
#include "Component.h"
#include <algorithm>

class ArmorComponent : public Component<ArmorComponent>
{
public:
    ArmorComponent() = default;
    ArmorComponent(int32 maxArmor, int32 currentArmor)
        : mBaseMaxArmor((std::max)(1, maxArmor))
        , mMaxArmor((std::max)(1, maxArmor))
        , mCurrentArmor((std::clamp)(currentArmor, 0, (std::max)(1, maxArmor)))
    {
    }

    int32 mBaseMaxArmor = 100; // 리듬 보너스를 뺀 순수 최대 방어막. 스폰 시 확정되고 이후 불변.
    int32 mMaxArmor = 100;     // 실효 최대 방어막 = mBaseMaxArmor + 보너스 (파생값)
    int32 mCurrentArmor = 100;

    bool IsBroken() const { return mCurrentArmor <= 0; }
};