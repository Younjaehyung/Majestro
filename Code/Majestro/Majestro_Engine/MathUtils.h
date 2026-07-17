#pragma once

namespace MathUtils
{
	inline constexpr float kPI = 3.1415926535f;
	inline constexpr float kDegToRad = 0.01745329251994329577f;

	Vec4 HlslQuatMul(const Vec4& q1, const Vec4& q2);
	Vec4 HlslQuatConj(const Vec4& q);
	Vec4 QuatFromAxisAngle(const Vec3& axis, float rad);

	// euler 각 변환
	Vec3 EulerDegreesToRadians(const Vec3& eulerDegrees);              // 도 -> 라디안(성분별)
	Quaternion QuatFromEulerDegrees(const Vec3& eulerDegrees);         // 도 -> 쿼터니언
	Vec3 ForwardFromEulerDegrees(const Vec3& eulerDegrees);            // 도 -> 로컬 +Z

	// 방향 -> 각도(도). yaw = XZ 평면 방위각, pitch = 상하각(위가 +)
	float YawDegreesFromDir(const Vec3& dir);
	float PitchDegreesFromDir(const Vec3& dir);

	Vec4 HlslQuatSlerp(const Vec4& a, const Vec4& b, float t);
	Vec4 LerpV4(const Vec4& a, const Vec4& b, float t);

	Vec4 MulCompV4(const Vec4& a, const Vec4& b);

	Vec4 DivCompV4(const Vec4& a, const Vec4& b);
	Vec4 MaxV4(const Vec4& a, const Vec4& b);

	float Saturate(float v);
	float Wrap180Degrees(float deg);
	float SmoothStep01(float t);
	float SmoothStep(float lo, float hi, float x);
	float LerpAngleDegrees(float startYaw, float targetYaw, float alpha);

	float EaseOutCubic(float t);
	float EaseOutBack(float t);
	float EaseInCubic(float t);
	float DampedSine(float t, float freq, float damp);

	// 난수 API
	std::mt19937& RandomEngine();
	float RandomRange(float minValue, float maxValue);
	int RandomInt(int minValue, int maxValue);
	std::size_t RandomIndex(std::size_t count);
}
