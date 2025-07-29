#pragma once

const ULONG MAX_SAMPLE_COUNT = 50; // Maximum frame time sample count

class Timer
{
public:
	Timer();
	virtual ~Timer();

	void Tick(float fLockFPS = 0.0f);
	void Start();
	void Stop();
	void Reset();

	unsigned long GetFrameRate(LPTSTR lpszString = NULL, int nCharacters = 0);
	float GetTimeElapsed();
	float GetTotalTime();


private:
	double							mTimeScale;
	float							mTimeElapsed;

	int64							mBasePerformanceCounter;
	int64							mPausedPerformanceCounter;
	int64							mStopPerformanceCounter;
	int64							mCurrentPerformanceCounter;
	int64							mLastPerformanceCounter;

	int64							mPerformanceFrequencyPerSec;

	float							mFrameTime[MAX_SAMPLE_COUNT];
	ULONG							mSampleCount;

	unsigned long					mCurrentFrameRate;
	unsigned long					mFramesPerSecond;
	float							mFPSTimeElapsed;

	bool							mStopped = false;
};
