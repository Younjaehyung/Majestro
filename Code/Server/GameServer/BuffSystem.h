#pragma once

#include "System.h"
#include "BuffComponent.h"

class BuffSystem : public System
{
public:
    explicit BuffSystem(World* world);

    void Update(float dt) override;

private:
    void ExecutePeriodicBuff(Entity target, BuffData& buff);
};