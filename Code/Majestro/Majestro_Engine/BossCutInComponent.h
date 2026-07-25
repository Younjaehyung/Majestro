#pragma once
#include "Component.h"

class BossCutInComponent : public Component<BossCutInComponent>
{
public:
    bool  mActive   = false;   // 컷인 재생 중
    float mElapsed  = 0.f;     // 컷인 시작 후 경과(초)
    int   mBossType = -1;
};
