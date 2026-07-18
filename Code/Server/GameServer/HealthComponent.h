#pragma once
#include "Component.h"
#include <algorithm>

class HealthComponent : public Component<HealthComponent>
{
public:
    HealthComponent() = default;
    HealthComponent(int32 maxHp, int32 currentHp)
        : mBaseMaxHp((std::max)(1, maxHp))
        , mMaxHp((std::max)(1, maxHp))
        , mCurrentHp((std::clamp)(currentHp, 0, (std::max)(1, maxHp)))
    {
    }

    int32 mBaseMaxHp = 100; // 리듬/버프 보너스를 뺀 순수 최대 체력. 스폰 시 확정되고 이후 불변.
    int32 mMaxHp = 100;     // 실효 최대 체력 = mBaseMaxHp + 보너스 (파생값, 매번 다시 계산됨)
    int32 mCurrentHp = 100;

    bool IsDead() const { return mCurrentHp <= 0; }
};