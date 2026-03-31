// Timer.cpp
#include "pch.h"
#include "Timer.h"

Timer::Timer()
{
    ::QueryPerformanceFrequency((LARGE_INTEGER*)&mPerformanceFrequencyPerSec);
    ::QueryPerformanceCounter((LARGE_INTEGER*)&mLastPerformanceCounter);
    mTimeScale = 1.0 / static_cast<double>(mPerformanceFrequencyPerSec);
    mBasePerformanceCounter = mLastPerformanceCounter;
}

void Timer::Tick(float fLockFPS)
{
    if (mStopped)
    {
        mTimeElapsed = 0.0f;
        return;
    }

    ::QueryPerformanceCounter((LARGE_INTEGER*)&mCurrentPerformanceCounter);
    float fTimeElapsed = static_cast<float>(
        (mCurrentPerformanceCounter - mLastPerformanceCounter) * mTimeScale);


    if (fLockFPS > 0.0f)
    {
        const float targetTime = 1.0f / fLockFPS;
        while (fTimeElapsed < targetTime)
        {
            
            ::QueryPerformanceCounter((LARGE_INTEGER*)&mCurrentPerformanceCounter);
            fTimeElapsed = static_cast<float>(
                (mCurrentPerformanceCounter - mLastPerformanceCounter) * mTimeScale);
        }
    }

    mLastPerformanceCounter = mCurrentPerformanceCounter;


    if (fTimeElapsed < 0.1f)
    {

        mFrameTime[mWriteIndex % MAX_SAMPLE_COUNT] = fTimeElapsed;
        ++mWriteIndex;
        if (mSampleCount < MAX_SAMPLE_COUNT) ++mSampleCount;
    }

    ++mFramesPerSecond;
    mFPSTimeElapsed += fTimeElapsed;
    if (mFPSTimeElapsed > 1.0f)
    {
        mCurrentFrameRate = mFramesPerSecond;
        mFramesPerSecond = 0;
        mFPSTimeElapsed = 0.0f;
    }

    mTimeElapsed = 0.0f;
    for (ULONG i = 0; i < mSampleCount; i++) mTimeElapsed += mFrameTime[i];
    if (mSampleCount > 0) mTimeElapsed /= static_cast<float>(mSampleCount);
}

void Timer::Start()
{
    if (mStopped)
    {
        int64_t nPerformanceCounter = 0;
        ::QueryPerformanceCounter((LARGE_INTEGER*)&nPerformanceCounter);
        // [수정] __int64 → int64_t 통일
        mPausedPerformanceCounter += (nPerformanceCounter - mStopPerformanceCounter);
        mLastPerformanceCounter = nPerformanceCounter;
        mStopPerformanceCounter = 0;
        mStopped = false;
    }
}

void Timer::Stop()
{
    if (!mStopped)
    {
        ::QueryPerformanceCounter((LARGE_INTEGER*)&mStopPerformanceCounter);
        mStopped = true;
    }
}

void Timer::Reset()
{
    int64_t nPerformanceCounter = 0;  // [수정] __int64 → int64_t
    ::QueryPerformanceCounter((LARGE_INTEGER*)&nPerformanceCounter);
    mBasePerformanceCounter = nPerformanceCounter;
    mLastPerformanceCounter = nPerformanceCounter;
    mStopPerformanceCounter = 0;
    mStopped = false;
}

unsigned long Timer::GetFrameRate(LPTSTR lpszString, int nCharacters)
{
    if (lpszString)
    {
        _itow_s(mCurrentFrameRate, lpszString, nCharacters, 10);
        // [수정] " FPS)" → " FPS" (닫는 괄호 제거, 의도에 따라 조정)
        wcscat_s(lpszString, nCharacters, _T(" FPS"));
    }
    return mCurrentFrameRate;
}

float Timer::GetTimeElapsed() const
{
    return mTimeElapsed;
}

float Timer::GetTotalTime() const
{
    // [수정] 수식 명확화: (현재 - 기준) - 누적정지
    if (mStopped)
        return static_cast<float>(
            ((mStopPerformanceCounter - mBasePerformanceCounter) - mPausedPerformanceCounter)
            * mTimeScale);

    return static_cast<float>(
        ((mCurrentPerformanceCounter - mBasePerformanceCounter) - mPausedPerformanceCounter)
        * mTimeScale);
}