#pragma once
#include "World.h"
#include "System.h"

class GamePhaseSystem : public System
{
public:
	GamePhaseSystem(World* world) : System(world) { mPhase = SysPhase::Pre; }
	void Initialize();
	void Update(float deltaTime);
private:
	void ConsumePreparePhase(float deltaTime, Entity e);

	// 점령 구역 마커 VFX
	Entity EnsureConquestMarker();
	void   UpdateConquestZoneMarker(class GameConquestComponent* conquest);
	void   StopConquestZoneMarker();

private:
	Entity mConquestMarker;          // 점령 구역 비콘 엔티티 (첫 Conquest 진입 시 생성, 씬 동안 재사용)
	uint8  mShownConquestZoneId = 0; // 현재 마커가 표시 중인 점령 구역 번호 (0=꺼짐)
};

