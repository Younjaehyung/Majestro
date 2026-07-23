#pragma once
#include "System.h"
#include "World.h"
#include "SimpleMath.h"

class CloudDriftComponent;
class TransformComponent;

// 광장(비행정) 배경의 구름 빌보드를 배의 진행 축(전후 = Z)만을 따라 이동시키고,
// 매 프레임 카메라를 향하도록(yaw 빌보드) 세워 항상 정면(구름 이미지)이 보이게 한다.
// per-entity 위치는 TransformComponent 가, 공용 이동 파라미터는 이 시스템이 소유한다.
class CloudDriftSystem : public System
{
public:
    explicit CloudDriftSystem(World* world) : System(world) { mPhase = SysPhase::Sim; }

    // 한 바퀴 순환 거리(전후 이동 폭). 이 거리를 지나면 레인 앞으로 되돌아온다.
    static constexpr float kRecycleDist = 9000.f;

    void Update(float dt) override;

private:
    void ApplyDrift(CloudDriftComponent* cloud, TransformComponent* tr, const Vec3& camPos, float dt);

private:
    // 진행 방향 = 배(forward +Z)가 전진 → 구름은 상대적으로 -Z 로 흐른다.
    const Vec3  kTravelDir = Vec3(0.f, 0.f, -1.f);

    // 기본 이동 속도(units/s). 개체별 mSpeedScale 로 변주.
    const float kBaseSpeed = 130.f;

    // 카드 앞면 보정: 메시 앞면이 로컬 +Z 면 0, 뒤집혀 보이면 180 으로.
    const float kFrontFlipDeg = 0.f;
};
