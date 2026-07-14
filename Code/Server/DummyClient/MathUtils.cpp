#include "pch.h"
#include "MathUtils.h"

std::mt19937& RandomEngine()
{
	// 더미 클라이언트 이동 시뮬레이션이 공유하는 난수 엔진이다.
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
