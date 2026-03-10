#pragma once
#include "pch.h"
#include "Component.h"
#include <algorithm>

class ArmorComponent : public Component<ArmorComponent>
{
public:
    ArmorComponent() = default;
    ArmorComponent(int32 maxArmor, int32 currentArmor)
        : mMaxArmor((std::max)(1, maxArmor))
        , mCurrentArmor((std::clamp)(currentArmor, 0, (std::max)(1, maxArmor)))
    {
    }

    int32 mMaxArmor = 100;
    int32 mCurrentArmor = 100;

    bool IsBroken() const { return mCurrentArmor <= 0; }
};