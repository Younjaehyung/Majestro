#pragma once
#include "System.h"

class HighlightComponent;


class HighlightSystem : public System
{
public:
    explicit HighlightSystem(World* world);

    void Update(float deltaTime) override;

private:
    void UpdateHighlights(float deltaTime);


    void FireBurst(Entity target, const HighlightComponent& hl);

    void SpawnSpark(Entity target, const HighlightComponent& hl);        // 상승 오라/기둥
    void SpawnGroundShock(Entity target, const HighlightComponent& hl);  // 지면 충격파 데칼
};
