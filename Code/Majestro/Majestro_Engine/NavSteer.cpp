#include "pch.h"
#include "NavSteer.h"
#include "NavMeshLoader.h"
#include "MovementComponent.h"
#include "MathUtils.h"

// Unity 빌드라 익명 namespace 가 다른 .cpp 와 합쳐진다. 이름이 겹치지 않게 따로 둔다.
namespace NavSteerDetail
{
	constexpr float kPathReplanSec = 0.3f;
	constexpr float kWaypointReachDistance = 80.f;	// 경로 지점에 이만큼 붙으면 다음 지점으로 (cm)
	constexpr float kMoveToleranceDeg = 60.f;		// 진행 방향 오차가 이 안이면 전진 입력
	constexpr long  kMaxMousePixels = 40;			// 한 프레임에 주입하는 마우스 이동 한계

	float HorizontalDistance(const Vec3& a, const Vec3& b)
	{
		const float dx = a.x - b.x;
		const float dz = a.z - b.z;
		return std::sqrt(dx * dx + dz * dz);
	}
}

NavSteer::~NavSteer()
{
	Release();
}

void NavSteer::Release()
{
	if (mNavQuery)
		dtFreeNavMeshQuery(mNavQuery);
	mNavQuery = nullptr;
	mNavMesh.reset();
}

const wchar_t* NavSteer::NavFile(SceneId id)
{
	switch (id)
	{
	case SceneId::FirstGame:  return L"all_tiles_navmesh.bin";
	case SceneId::SecondGame: return L"MapDesert_navmesh.bin";
	case SceneId::ThirdGame:  return L"Map003_navmesh.bin";
	case SceneId::FourthGame: return L"MapDragon_navmesh.bin";
	default:                  return nullptr;
	}
}

bool NavSteer::Load(SceneId sceneId)
{
	Release();
	mPathTimer = 0.f;

	const wchar_t* file = NavFile(sceneId);
	if (file == nullptr)
		return false;

	// ponytail: 서버 리소스 폴더를 직접 읽는다 (클라이언트에는 스테이지별 NavMesh 가 없다).
	//           배포본에서 쓰려면 클라이언트 Resources 로 복사하고 경로를 바꾼다.
	auto navMesh = std::make_shared<NavMesh>();
	navMesh->Load(std::wstring(L"..\\..\\Server\\Resources\\NavMesh\\") + file);
	dtNavMeshQuery* query = navMesh->mDtNavMesh ? dtAllocNavMeshQuery() : nullptr;
	if (query == nullptr || dtStatusFailed(query->init(navMesh->mDtNavMesh, 2048)))
	{
		if (query)
			dtFreeNavMeshQuery(query);
		return false;
	}
	mNavMesh = navMesh;
	mNavQuery = query;
	return true;
}

Vec3 NavSteer::PathStep(const Vec3& position, const Vec3& target)
{
	if (mNavQuery == nullptr)
		return target;

	// 엔진(cm) -> NavMesh(m), 축 교환 포함 (서버 NavMeshLoader 와 같은 규칙)
	const float start[3] = { position.z * 0.01f, position.y * 0.01f, position.x * 0.01f };
	const float end[3] = { target.z * 0.01f, target.y * 0.01f, target.x * 0.01f };
	const float extents[3] = { 5.f, 10.f, 5.f };
	const dtQueryFilter filter;

	dtPolyRef startRef = 0, endRef = 0;
	float startOnMesh[3], endOnMesh[3];
	mNavQuery->findNearestPoly(start, extents, &filter, &startRef, startOnMesh);
	mNavQuery->findNearestPoly(end, extents, &filter, &endRef, endOnMesh);
	if (!startRef || !endRef)
		return target;

	dtPolyRef polys[256];
	int polyCount = 0;
	if (dtStatusFailed(mNavQuery->findPath(startRef, endRef, startOnMesh, endOnMesh, &filter, polys, &polyCount, 256))
		|| polyCount == 0)
		return target;

	float corners[8 * 3];
	int cornerCount = 0;
	mNavQuery->findStraightPath(startOnMesh, endOnMesh, polys, polyCount, corners, nullptr, nullptr, &cornerCount, 8);
	for (int i = 1; i < cornerCount; ++i)	// 0 번은 출발점
	{
		const Vec3 corner(corners[i * 3 + 2] * 100.f, corners[i * 3 + 1] * 100.f, corners[i * 3] * 100.f);
		if (NavSteerDetail::HorizontalDistance(position, corner) > NavSteerDetail::kWaypointReachDistance)
			return corner;
	}
	return target;
}

void NavSteer::Steer(InputFrame& frame, const PlayerMovementComponent& movement,
	const Vec3& position, const Vec3& target, float dt, bool move)
{
	// 이동할 때는 NavMesh 경로의 다음 지점을 향한다 (제자리 조준은 대상을 직접 본다).
	Vec3 aim = target;
	if (move)
	{
		mPathTimer -= dt;
		if (mPathTimer <= 0.f)
		{
			mPathTimer = NavSteerDetail::kPathReplanSec;
			mPathNext = PathStep(position, target);
		}
		aim = mPathNext;
	}
	else
	{
		mPathTimer = 0.f;
	}

	// 카메라 yaw 를 목표 방향으로, pitch 는 수평으로 맞춘다. 마우스 1픽셀이 0.5도라 오차가 매 프레임 절반씩 준다.
	const float targetYaw = MathUtils::YawDegreesFromDir(aim - position);
	const float yawError = MathUtils::Wrap180Degrees(targetYaw - movement.mTargetRotation.y);
	frame.mouseDelta.x = std::clamp(std::lround(yawError), -NavSteerDetail::kMaxMousePixels, NavSteerDetail::kMaxMousePixels);
	frame.mouseDelta.y = std::clamp(std::lround(-movement.mTargetRotation.x), -NavSteerDetail::kMaxMousePixels, NavSteerDetail::kMaxMousePixels);

	if (move && std::fabs(yawError) < NavSteerDetail::kMoveToleranceDeg)
		frame.keys[static_cast<size_t>(eKeyCode::W)] = true;
}
