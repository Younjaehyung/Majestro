#pragma once

#include <algorithm>
#include <cstddef>
#include <random>

namespace MathUtils
{
	// 난수
	std::mt19937& RandomEngine();
	float RandomRange(float minValue, float maxValue);
	int32 RandomInt(int32 minValue, int32 maxValue);
	std::size_t RandomIndex(std::size_t count);
	Vec3 SampleDiskXZ(float radius);


	float YawDegreesFromDir(const Vec3& dir);
	float PitchDegreesFromDir(const Vec3& dir);
	Vec3 EulerDegreesFromForward(const Vec3& forward);
	Vec3 ForwardFromYawPitchDegrees(float yawDeg, float pitchDeg);

	// 수평 forward / 오른쪽 벡터
	Vec3 SafeHorizontalForward(Vec3 forward);
	Vec3 SafeRightFromForward(const Vec3& forward);
}
