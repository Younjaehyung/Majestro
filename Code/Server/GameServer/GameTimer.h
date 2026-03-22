#pragma once

#include "Timer.h"

extern Timer gTimer;

inline float GetServerTotalTimeSeconds()
{
    return gTimer.GetTotalTime();
}

inline float GetServerDeltaTimeSeconds()
{
    return gTimer.GetTimeElapsed();
}