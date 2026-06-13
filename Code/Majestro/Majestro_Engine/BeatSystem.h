#pragma once
#include "World.h"
#include "System.h"

class ControllerComponent;
class MainPlayerComponent;

class BeatSystem : public System
{
public:
	BeatSystem(World* world);

	void Initialize();
	void Update(float dt);

	// 공유 Song Clock: 서버 곡 위치로 로컬 곡 시간을 정렬.
	void SyncSongPosition(float serverSongPos);		// 큰 오차는 즉시 스냅, 작은 오차는 서서히 보정

	// 서버 동기화된 박자 시계 위치
	float GetSongPosition() const { return mSeconds; }

	// 곡 시작부터 지금까지 박자가 몇 번 지나갔나
	int64 GetAbsoluteBeatIndex() const { return static_cast<int64>(mSeconds / mBpmSeconds); }

	// 한 박자 길이(박자 입력 판정 계산용)
	float GetBeatSeconds() const { return mBpmSeconds; }

	// 서버 곡위치 sync 를 한 번이라도 받았는지 (곡 시작 T0 정렬 타이밍 판정)
	bool HasSynced() const { return mHasSynced; }

private:
	bool mHasSynced = false; // SyncSongPosition 최초 수신 여부

	int mBpm = 160; 
	int mBeat = 0;
	int mLastFiredBeat = -1; // 마지막으로 이벤트를 발행한 박자 번호

	float mBpmSeconds = 60.f / mBpm;	// 한 박자 길이
	float mSeconds = 0.0f;	// 곡 시작부터의 경과 시간(서버랑 동기화)
	float mBonusTime = 0.2f;

	// 동기화 보정 파라미터
	float mResyncSnapThreshold = 0.08f; // 이보다 큰 오차는 즉시 스냅
	float mDriftCorrectGain = 0.10f;    // 작은 오차의 프레임당 보정 비율
};
