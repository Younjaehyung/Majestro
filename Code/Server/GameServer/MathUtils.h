#pragma once

#include <algorithm>
#include <cstddef>
#include <random>

std::mt19937& RandomEngine();
float RandomRange(float minValue, float maxValue);
int32 RandomInt(int32 minValue, int32 maxValue);
std::size_t RandomIndex(std::size_t count);
Vec3 SampleDiskXZ(float radius);
