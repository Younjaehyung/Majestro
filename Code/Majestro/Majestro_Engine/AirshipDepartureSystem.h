#pragma once
#include "System.h"
#include "World.h"

class AirshipDepartureComponent;

class AirshipDepartureSystem : public System
{
public:
    AirshipDepartureSystem(World* w) : System(w) { mPhase = SysPhase::Post; }

    std::vector<std::type_index> After() const override;  // CameraSystem 뒤에서 최종 오버라이드

    void Update(float dt) override;

private:

    void Apply(AirshipDepartureComponent* dep, float dt);

    // 재생 종료 + 예약된 씬 전환
    void Finish(AirshipDepartureComponent* dep);
};
