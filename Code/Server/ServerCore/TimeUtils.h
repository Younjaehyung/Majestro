#pragma once

#include <chrono>

inline float GetSteadyTimeSeconds()
{
    const auto now = std::chrono::steady_clock::now().time_since_epoch();
    return std::chrono::duration<float>(now).count();
}