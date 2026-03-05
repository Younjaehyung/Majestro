#pragma once
#include "Component.h"
#include "EnginePch.h"
#include <algorithm>

class HealthComponent : public Component<HealthComponent>
{
public:
    HealthComponent() = default;
    HealthComponent(int32 maxHp, int32 currentHp)
        : mMaxHp((std::max)(1, maxHp))
        , mCurrentHp((std::clamp)(currentHp, 0, (std::max)(1, maxHp)))
    {
    }

    int32 mMaxHp = 100;
    int32 mCurrentHp = 100;

    bool IsDead() const { return mCurrentHp <= 0; }
};