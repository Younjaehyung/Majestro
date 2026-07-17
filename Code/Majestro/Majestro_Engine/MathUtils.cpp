#include "pch.h"
#include "MathUtils.h"

namespace MathUtils
{

std::mt19937& RandomEngine()
{
	static thread_local std::mt19937 engine{ std::random_device{}() };
	return engine;
}

float RandomRange(float minValue, float maxValue)
{
	if (maxValue < minValue)
		std::swap(minValue, maxValue);

	std::uniform_real_distribution<float> dist(minValue, maxValue);
	return dist(RandomEngine());
}

int RandomInt(int minValue, int maxValue)
{
	if (maxValue < minValue)
		std::swap(minValue, maxValue);

	std::uniform_int_distribution<int> dist(minValue, maxValue);
	return dist(RandomEngine());
}

std::size_t RandomIndex(std::size_t count)
{
	if (count == 0)
		return 0;

	std::uniform_int_distribution<std::size_t> dist(0, count - 1);
	return dist(RandomEngine());
}

Vec3 EulerDegreesToRadians(const Vec3& eulerDegrees)
{
	return eulerDegrees * kDegToRad;
}

Quaternion QuatFromEulerDegrees(const Vec3& eulerDegrees)
{
	// euler(x=pitch, y=yaw, z=roll) 도 -> 쿼터니언
	return Quaternion::CreateFromYawPitchRoll(
		eulerDegrees.y * kDegToRad,
		eulerDegrees.x * kDegToRad,
		eulerDegrees.z * kDegToRad);
}

Vec3 ForwardFromEulerDegrees(const Vec3& eulerDegrees)
{
	// 로컬 +Z
	return Vec3::Transform(Vec3::Backward, QuatFromEulerDegrees(eulerDegrees));
}

float YawDegreesFromDir(const Vec3& dir)
{
	return DirectX::XMConvertToDegrees(atan2f(dir.x, dir.z));
}

float PitchDegreesFromDir(const Vec3& dir)
{
	return DirectX::XMConvertToDegrees(-asinf(std::clamp(dir.y, -1.f, 1.f)));
}

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

float SmoothStep(float lo, float hi, float x)
{
	if (hi <= lo) return x >= hi ? 1.f : 0.f;
	return SmoothStep01((x - lo) / (hi - lo));
}

float LerpAngleDegrees(float startYaw, float targetYaw, float alpha)
{
	const float deltaYaw = Wrap180Degrees(targetYaw - startYaw);
	return startYaw + deltaYaw * std::clamp(alpha, 0.f, 1.f);
}

//처음엔 빠르게 움직이다가 목적지에 다가갈수록 천천히 감속
float EaseOutCubic(float t)
{
	float u = 1.f - t;
	return 1.f - u * u * u;
}

// 처음엔 빠르게 움직이다가 목적지에 다가갈수록 천천히 감속 + 약간의 오버슈트
// 약간 지나쳤다가 다시 백
float EaseOutBack(float t)
{
	// c1 = 1.70158 : 약 10% 오버슈트.
	const float c1 = 1.70158f;
	const float c3 = c1 + 1.f;
	const float u = t - 1.f;
	return 1.f + c3 * u * u * u + c1 * u * u;
}

// 처음엔 천천히 움직이다가 목적지에 다가갈수록 빠르게 가속
float EaseInCubic(float t)
{
	return t * t * t;
}

float DampedSine(float t, float freq, float damp)
{
	// 바운스/Shake 의 진동
	// 감쇠 사인
	return std::sin(t * freq) * std::exp(-damp * t);
}

} // namespace MathUtils
