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
    void CameraPunch(const HighlightComponent& hl);                      // 카메라 흔들림 + 줌아웃 펀치
    bool IsLocalCaster(Entity target) const;                            // 이 엔티티가 로컬 시전자인지
};
