#pragma once
#include "System.h"
#include "World.h"
#include "SimpleMath.h"

class CloudDriftComponent;
class TransformComponent;

class CloudDriftSystem : public System
{
public:
    explicit CloudDriftSystem(World* world) : System(world) { mPhase = SysPhase::Sim; }

    // 한 바퀴 순환 거리(전후 이동 폭). 이 거리를 지나면 레인 앞으로 되돌아온다.
    static constexpr float kRecycleDist = 9000.f;

    void Update(float dt) override;

private:
    void ApplyDrift(CloudDriftComponent* cloud, TransformComponent* tr, float dt);

private:
    // 진행 방향 = 배(forward +Z)가 전진 → 구름은 상대적으로 -Z 로 흐른다.
    const Vec3  kTravelDir = Vec3(0.f, 0.f, -1.f);

    // 기본 이동 속도(units/s). 개체별 mSpeedScale 로 변주.
    const float kBaseSpeed = 130.f;
};
