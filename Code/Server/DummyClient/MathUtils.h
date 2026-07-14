#pragma once

#include <algorithm>
#include <random>

// 더미 클라이언트 랜덤 API를 한 곳에서 관리해 파일 전역 mt19937 중복을 막는다.
std::mt19937& RandomEngine();
float RandomRange(float minValue, float maxValue);
