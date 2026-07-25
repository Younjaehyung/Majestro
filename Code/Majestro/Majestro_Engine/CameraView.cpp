#include "pch.h"
#include "CameraView.h"
#include "JsonUtils.h"
#include "World.h"
#include "TagComponent.h"
#include "CameraComponent.h"
#include "TransformComponent.h"
#include "IntroSequenceComponent.h"
#include "AirshipDepartureComponent.h"
#include "BossCinematicComponent.h"
#include "BossCutInComponent.h"
#include "EventManager.h"
#include "GameEvents.h"

namespace Cinematic
{

void ConvertMainMenuSample(const json& s, CameraView& v)
{
	v.position = ConvertUeVectorToDx(ParseVec3ArrayOrObject(RequireJson(s, "position"), 1.f));

	const json& b = RequireJson(s, "basis");
	Vec3 dxF = ConvertUeVectorToDx(ParseVec3ArrayOrObject(RequireJson(b, "forward"), 1.f));
	Vec3 dxR = ConvertUeVectorToDx(ParseVec3ArrayOrObject(RequireJson(b, "right"), 1.f));
	Vec3 dxU = ConvertUeVectorToDx(ParseVec3ArrayOrObject(RequireJson(b, "up"), 1.f));

	Matrix m = Matrix::Identity;
	m.Right(dxR);
	m.Up(dxU);
	m.Backward(dxF);
	v.rotation = Quaternion::CreateFromRotationMatrix(m);
	v.rotation.Normalize();

	v.fovDeg = GetFloat(s, "fov");
}

CameraView SampleCameraSequence(const std::vector<CameraKeyframe>& keys, float t)
{
	CameraView view;
	if (keys.empty())
		return view;

	if (t <= keys.front().seconds)
		return keys.front().view;
	if (t >= keys.back().seconds)
		return keys.back().view;

	size_t i = 0;
	while (i + 1 < keys.size() && keys[i + 1].seconds <= t)
		++i;

	const CameraKeyframe& a = keys[i];
	const CameraKeyframe& b = keys[i + 1];
	const float span = b.seconds - a.seconds;
	const float u = (span > 1e-5f) ? (t - a.seconds) / span : 0.f;

	view.position = Vec3::Lerp(a.view.position, b.view.position, u);
	view.rotation = Quaternion::Slerp(a.view.rotation, b.view.rotation, u);
	view.fovDeg   = a.view.fovDeg + (b.view.fovDeg - a.view.fovDeg) * u;
	return view;
}

bool IsSameMainMenuStop(const CameraView& a, const CameraView& b)
{
	if (Vec3::Distance(a.position, b.position) >= 0.5f) return false;
	// dot 의 절대값이 1에 가까우면 같음 (q, -q 모두 동일 회전)
	if (std::abs(a.rotation.Dot(b.rotation)) < 0.9999f) return false;
	if (std::abs(a.fovDeg - b.fovDeg) >= 0.01f) return false;
	return true;
}

void ApplyCameraSequence(World* world, const std::vector<CameraKeyframe>& keys, float elapsed)
{
	if (world == nullptr)
		return;

	// 활성 카메라 엔티티(로컬 전용) 조회
	auto cams = world->GetEntitiesWithComponents<MainCameraComponent, CameraComponent, TransformComponent>();
	if (cams.empty())
		return;

	const Entity camE = cams.front();
	CameraComponent*    cam = world->GetComponent<CameraComponent>(camE);
	TransformComponent* tr  = world->GetComponent<TransformComponent>(camE);
	if (!cam || !tr)
		return;

	// 키프레임 구간 보간
	const CameraView view = SampleCameraSequence(keys, elapsed);

	// Transform 적용
	tr->mLocalPosition = view.position;
	const Vec3 e = view.rotation.ToEuler();
	tr->mLocalRotationE = Vec3(XMConvertToDegrees(e.x),
	                           XMConvertToDegrees(e.y),
	                           XMConvertToDegrees(e.z));

	// FOV
	const float aspect  = cam->mWidth / cam->mHeight;
	const float hFovRad = XMConvertToRadians(view.fovDeg);
	const float vFovRad = 2.f * atanf(tanf(hFovRad * 0.5f) / aspect);
	cam->SetFOV(vFovRad);


	tr->FinalUpdate();
	cam->FinalUpdate(tr->GetWorldMatrix().Invert());
}

bool IsAnyCinematicPlaying(World* world)
{
	if (world == nullptr)
		return false;

	const Entity singleton = world->GetSingletonEntity();

	// 씬 진입
	if (world->HasComponentPool<IntroSequenceComponent>())
	{
		const IntroSequenceComponent* seq = world->GetComponent<IntroSequenceComponent>(singleton);
		if (seq != nullptr && seq->mPlaying)
			return true;
	}

	// 비행정
	if (world->HasComponentPool<AirshipDepartureComponent>())
	{
		const AirshipDepartureComponent* dep = world->GetComponent<AirshipDepartureComponent>(singleton);
		if (dep != nullptr && dep->mPlaying)
			return true;
	}

	// 보스
	if (world->HasComponentPool<BossCinematicComponent>())
	{
		const BossCinematicComponent* boss = world->GetComponent<BossCinematicComponent>(singleton);
		if (boss != nullptr && boss->mPlaying)
			return true;
	}

	// 보스 컷인
	if (world->HasComponentPool<BossCutInComponent>())
	{
		const BossCutInComponent* cutIn = world->GetComponent<BossCutInComponent>(singleton);
		if (cutIn != nullptr && cutIn->mActive)
			return true;
	}

	return false;
}

void RequestPlazaLevelEnter(World* world, SceneId target)
{
	if (world == nullptr)
		return;

	if (world->HasComponentPool<AirshipDepartureComponent>())
	{
		AirshipDepartureComponent* departure = world->GetSingleton<AirshipDepartureComponent>();
		if (departure != nullptr && departure->HasSequence() && !departure->mPlaying)
		{
			departure->Arm(target);
			return;
		}
	}

	world->GetEventManager()->Enqueue(EvNetSceneChange{ target });
}

}