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

    // 한 바퀴 순환 거리
    static constexpr float kRecycleDist = 9000.f;

    void Update(float dt) override;

private:
    void ApplyDrift(CloudDriftComponent* cloud, TransformComponent* tr, float dt);

private:
    // 진행 방향
    const Vec3  kTravelDir   = Vec3(0.f, 0.f, -1.f);

    // 기본 이동 속도
    const float kBaseSpeed   = 130.f;
};
