// Timer.h
#pragma once
#include <cstdint>  // [수정] int64_t 표준 타입 사용

const ULONG MAX_SAMPLE_COUNT = 50;

class Timer
{
public:
    Timer();
    virtual ~Timer() = default;  // [수정] 빈 소멸자 제거, default 사용
    void Tick(float fLockFPS = 0.0f);
    void Start();
    void Stop();
    void Reset();
    unsigned long GetFrameRate(LPTSTR lpszString = nullptr, int nCharacters = 0);
    float GetTimeElapsed() const;  // [수정] const 명시
    float GetTotalTime() const;    // [수정] const 명시

private:
    double          mTimeScale = 0.0;
    float           mTimeElapsed = 0.0f;
    int64_t         mBasePerformanceCounter = 0;    // [수정] int64 → int64_t 통일
    int64_t         mPausedPerformanceCounter = 0;
    int64_t         mStopPerformanceCounter = 0;
    int64_t         mCurrentPerformanceCounter = 0;
    int64_t         mLastPerformanceCounter = 0;
    int64_t         mPerformanceFrequencyPerSec = 0;
    float           mFrameTime[MAX_SAMPLE_COUNT] = {};
    ULONG           mSampleCount = 0;
    ULONG           mWriteIndex = 0;    // [추가] 링 버퍼 인덱스
    unsigned long   mCurrentFrameRate = 0;
    unsigned long   mFramesPerSecond = 0;
    float           mFPSTimeElapsed = 0.0f;
    bool            mStopped = false;
};