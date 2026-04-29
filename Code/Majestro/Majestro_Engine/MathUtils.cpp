#include "pch.h"
#include "MathUtils.h"


Vec4 HlslQuatMul(const Vec4& q1, const Vec4& q2)
{
	XMVECTOR r = XMQuaternionMultiply(q2, q1);


	const float n2 = XMVectorGetX(XMQuaternionLengthSq(r));
	if (n2 > 1e-12f)
	{
		r = XMQuaternionNormalize(r);
	}

	return Vec4(r);
}

Vec4 QuatFromAxisAngle(const Vec3& axis, float rad)
{
	Vec3 n = axis;
	if (n.LengthSquared() <= 0.0001f)
		return Vec4(0.f, 0.f, 0.f, 1.f);

	n.Normalize();

	const float half = rad * 0.5f;
	const float s = sinf(half);
	return Vec4(n.x * s, n.y * s, n.z * s, cosf(half));
}


Vec4 HlslQuatConj(const Vec4& q)
{
	return Vec4(-q.x, -q.y, -q.z, q.w);
}

// HLSL QuaternionSlerp
Vec4 HlslQuatSlerp(const Vec4& a, const Vec4& b, float t)
{
	return Vec4(XMQuaternionSlerp(a, b, t));
}


Vec4 LerpV4(const Vec4& a, const Vec4& b, float t)
{
	return Vec4(XMVectorLerp(a, b, t));
}


Vec4 MulCompV4(const Vec4& a, const Vec4& b)
{
	return Vec4(a.x * b.x, a.y * b.y, a.z * b.z, a.w * b.w);
}

Vec4 DivCompV4(const Vec4& a, const Vec4& b)
{
	return Vec4(a.x / b.x, a.y / b.y, a.z / b.z, a.w / b.w);
}

Vec4 MaxV4(const Vec4& a, const Vec4& b)
{
	return Vec4(max(a.x, b.x), max(a.y, b.y), max(a.z, b.z), max(a.w, b.w));
}


float Saturate(float v)
{
	return std::clamp(v, 0.f, 1.f);
}

float Wrap180Degrees(float deg)
{
	deg = fmodf(deg + 180.f, 360.f);
	if (deg < 0.f)
		deg += 360.f;
	return deg - 180.f;
}

float SmoothStep01(float t)
{
	t = std::clamp(t, 0.f, 1.f);
	return t * t * (3.f - 2.f * t);
}

float LerpAngleDegrees(float startYaw, float targetYaw, float alpha)
{
	const float deltaYaw = Wrap180Degrees(targetYaw - startYaw);
	return startYaw + deltaYaw * std::clamp(alpha, 0.f, 1.f);
}

float EaseOutCubic(float t)
{
	float u = 1.f - t;
	return 1.f - u * u * u;
}