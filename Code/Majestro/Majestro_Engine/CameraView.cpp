#include "pch.h"
#include "CameraView.h"
#include "JsonUtils.h"

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

}