#pragma once
#include "System.h"

class RhythmSystem : public System
{
public:
    RhythmSystem(World* world);

    void Update(float deltaTime) override;

private:
    
	void UpdateEmissives(float deltaTime);
};
