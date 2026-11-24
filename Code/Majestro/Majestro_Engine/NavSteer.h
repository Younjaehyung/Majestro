#pragma once
#include <memory>
#include "InputManager.h"

class NavMesh;
class dtNavMeshQuery;
class PlayerMovementComponent;

// 입력 주입으로 플레이어를 목표 지점까지 걷게 한다 (QaBot, MCP move_to 가 같이 쓴다).
//   스테이지 NavMesh 경로의 다음 지점으로 카메라 yaw 를 마우스로 돌리고, 방향이 맞으면 W 를 누른다.
//   끼임 처리(점프·옆걸음)와 기록은 쓰는 쪽이 한다.
class NavSteer
{
public:
	~NavSteer();

	// 서버 ResourceManager 가 스테이지별로 여는 NavMesh 와 같은 파일. 없는 씬이면 nullptr.
	static const wchar_t* NavFile(SceneId id);

	// 씬의 NavMesh 를 연다. 실패하거나 파일이 없는 씬이면 false 이고, 이후 Steer 는 직선으로 간다.
	bool Load(SceneId sceneId);

	// target 까지의 경로에서 다음으로 향할 지점. 경로가 없으면 target 그대로.
	Vec3 PathStep(const Vec3& position, const Vec3& target);

	// frame 에 마우스 이동(yaw 정렬, pitch 수평)을 넣고, move 이고 방향이 맞으면 W 도 넣는다.
	// move=false 면 target 을 직접 바라보기만 한다.
	void Steer(InputFrame& frame, const PlayerMovementComponent& movement,
		const Vec3& position, const Vec3& target, float dt, bool move);

	// 경로 재계산 타이머를 비운다 (씬 전환, 목표 변경 시).
	void ResetPath() { mPathTimer = 0.f; }

private:
	void Release();

	std::shared_ptr<NavMesh> mNavMesh;
	dtNavMeshQuery* mNavQuery = nullptr;
	float mPathTimer = 0.f;
	Vec3 mPathNext{};
};
