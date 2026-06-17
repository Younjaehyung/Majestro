#pragma once

#include "Component.h"

class FlyComponent : public Component<FlyComponent>
{
public:
    float mHoverHeight = 250.0f;
    float mAttackHoverHeight = 45.0f;
    float mGround = 0.0f;
    float mMinGroundClearance = 120.0f;
    float mVerticalMoveSpeed = 700.0f;
    bool mDirectFlight = false;
    bool mAttackDiveActive = false;
    bool mRetreating = false;
    float mRetreatDistance = 1000.0f;
    float mRetreatSpeedMultiplier = 1.2f;
};
