#include "pch.h"
#include "CameraView.h"
#include "JsonUtils.h"

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

bool IsSameMainMenuStop(const CameraView& a, const CameraView& b)
{
	if (Vec3::Distance(a.position, b.position) >= 0.5f) return false;
	// dot 의 절대값이 1에 가까우면 같음 (q, -q 모두 동일 회전)
	if (std::abs(a.rotation.Dot(b.rotation)) < 0.9999f) return false;
	if (std::abs(a.fovDeg - b.fovDeg) >= 0.01f) return false;
	return true;
}