#pragma once
#include "World.h"
#include "System.h"

class Navigation;
class NavMesh;

class EnemySystem : public System
{
public:
    EnemySystem(World* world);

    void Initialize();

    void Update(float deltaTime) override;

	Vec3 PathFinder(const Vec3& from);  // 가장 가까운 플레이어 위치 탐색
private:

	std::vector<Vec3> mPlayerPositions;

	std::shared_ptr<NavMesh> mNavMesh; // NavMesh 리소스 생존 보장용

	// 임계값 상수
	static constexpr float ARRIVE_THRESHOLD_SQ = 4.0f * 100.f;    // 웨이포인트 도달 반경 (2m)
	static constexpr float RETARGET_THRESHOLD_SQ = 100.0f * 100.f; // 목적지 변경 감지 거리 (10m)
};
