#pragma once
#include "Component.h"
#include "SimpleMath.h"


class CloudDriftComponent : public Component<CloudDriftComponent>
{
public:
    // 원래 월드 위치(배의 양옆)
    Vec3  mBasePos = {};

    // 개체별 속도 배율
    float mSpeedScale = 1.f;

    // 진행 축 누적 이동 거리(순환 판정용)
    float mTravelled = 0.f;
};
