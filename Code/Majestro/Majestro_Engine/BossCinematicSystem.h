#pragma once
#include "System.h"
#include "World.h"

class BossCinematicComponent;

class BossCinematicSystem : public System
{
public:
    BossCinematicSystem(World* w) : System(w) { mPhase = SysPhase::Post; }

    std::vector<std::type_index> After() const override;

    void Update(float dt) override;

private:

    void Stop(BossCinematicComponent* seq);

private:
    uint8 mPrevPhase = 0;  // 직전 프레임 Phase
};
