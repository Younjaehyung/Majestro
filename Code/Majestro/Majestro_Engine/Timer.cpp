#include "pch.h"
#include "Timer.h"

Timer::Timer()
{
	::QueryPerformanceFrequency((LARGE_INTEGER*)&mPerformanceFrequencyPerSec);
	::QueryPerformanceCounter((LARGE_INTEGER*)&mLastPerformanceCounter);
	mTimeScale = 1.0 / (double)mPerformanceFrequencyPerSec;

	mBasePerformanceCounter = mLastPerformanceCounter;
	mPausedPerformanceCounter = 0;
	mStopPerformanceCounter = 0;

	mSampleCount = 0;
	mCurrentFrameRate = 0;
	mFramesPerSecond = 0;
	mFPSTimeElapsed = 0.0f;
}

Timer::~Timer()
{
}

void Timer::Tick(float fLockFPS)
{
	if (mStopped)
	{
		mTimeElapsed = 0.0f;
		return;
	}
	float fTimeElapsed;

	::QueryPerformanceCounter((LARGE_INTEGER*)&mCurrentPerformanceCounter);
	fTimeElapsed = float((mCurrentPerformanceCounter - mLastPerformanceCounter) * mTimeScale);

	if (fLockFPS > 0.0f)
	{
		while (fTimeElapsed < (1.0f / fLockFPS))
		{
			::QueryPerformanceCounter((LARGE_INTEGER*)&mCurrentPerformanceCounter);
			fTimeElapsed = float((mCurrentPerformanceCounter - mLastPerformanceCounter) * mTimeScale);
		}
	}

	mLastPerformanceCounter = mCurrentPerformanceCounter;

	if (fabsf(fTimeElapsed - mTimeElapsed) < 1.0f)
	{
		::memmove(&mFrameTime[1], mFrameTime, (MAX_SAMPLE_COUNT - 1) * sizeof(float));
		mFrameTime[0] = fTimeElapsed;
		if (mSampleCount < MAX_SAMPLE_COUNT) mSampleCount++;
	}

	mFramesPerSecond++;
	mFPSTimeElapsed += fTimeElapsed;
	if (mFPSTimeElapsed > 1.0f)
	{
		mCurrentFrameRate = mFramesPerSecond;
		mFramesPerSecond = 0;
		mFPSTimeElapsed = 0.0f;
	}

	mTimeElapsed = 0.0f;
	for (ULONG i = 0; i < mSampleCount; i++) mTimeElapsed += mFrameTime[i];
	if (mSampleCount > 0) mTimeElapsed /= mSampleCount;

}

void Timer::Start()
{
	__int64 nPerformanceCounter;
	::QueryPerformanceCounter((LARGE_INTEGER*)&nPerformanceCounter);
	if (mStopped)
	{
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
	__int64 nPerformanceCounter;
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
		wcscat_s(lpszString, nCharacters, _T(" FPS)"));
	}

	return(mCurrentFrameRate);
}

float Timer::GetTimeElapsed()
{
	return(mTimeElapsed);
}

float Timer::GetTotalTime()
{
	if (mStopped) return(float(((mStopPerformanceCounter - mPausedPerformanceCounter) - mBasePerformanceCounter) * mTimeScale));
	return(float(((mCurrentPerformanceCounter - mPausedPerformanceCounter) - mBasePerformanceCounter) * mTimeScale));

}
