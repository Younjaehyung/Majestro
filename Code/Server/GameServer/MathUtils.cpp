#include "pch.h"
#include "MathUtils.h"

#include <cmath>

Vec3 EulerDegreesFromForward(const Vec3& forward)
{
	Vec3 dir = forward;
	if (dir.LengthSquared() <= 1e-6f)
		dir = Vec3::Forward;
	dir.Normalize();

	float ny = -dir.y;
	ny = (ny < -1.0f) ? -1.0f : (ny > 1.0f ? 1.0f : ny);

	const float yawDeg   = std::atan2(dir.x, dir.z) * kRadToDeg;
	const float pitchDeg = std::asin(ny) * kRadToDeg;
	return Vec3(pitchDeg, yawDeg, 0.0f);
}

std::mt19937& RandomEngine()
{
	// 서버 로직에서 공용으로 사용하는 스레드별 난수 엔진이다.
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

int32 RandomInt(int32 minValue, int32 maxValue)
{
	if (maxValue < minValue)
		std::swap(minValue, maxValue);

	std::uniform_int_distribution<int32> dist(minValue, maxValue);
	return dist(RandomEngine());
}

std::size_t RandomIndex(std::size_t count)
{
	if (count == 0)
		return 0;

	std::uniform_int_distribution<std::size_t> dist(0, count - 1);
	return dist(RandomEngine());
}

Vec3 SampleDiskXZ(float radius)
{
	if (radius <= 0.0f)
		return Vec3(0.0f, 0.0f, 0.0f);

	const float angle = RandomRange(0.0f, 6.28318530718f);
	const float r = radius * std::sqrt(RandomRange(0.0f, 1.0f));
	return Vec3(std::cos(angle) * r, 0.0f, std::sin(angle) * r);
}
