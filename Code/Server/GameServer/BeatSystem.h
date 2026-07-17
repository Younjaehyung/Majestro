#pragma once
#include <vector>

#include "World.h"
#include "System.h"
#include "GameEvents.h"
#include "Protocol/RhythmMusicDefinitions.h"


class ControllerComponent;
class MainPlayerComponent;

class BeatSystem : public System
{
public:
	BeatSystem(World* world);

	void Initialize();
	void Update(float dt);
	void SyncAllRhythmBuffsNow();

private:
	void CollectPendingBuffRequests();
	void ApplyPendingBuffRequests();

public:
	const float mBpmSeconds = kMusicBeatSeconds;

	// 서버 권위 곡 진행 시간(초) — 공유 Song Clock 동기에 사용.
	float GetSongPosition() const { return mSeconds; }

	// 모듈로 전의 절대 박자 인덱스 — 리듬 전환 look-ahead 스케줄에 사용.
	int64 GetAbsoluteBeatIndex() const { return static_cast<int64>(mSeconds / mBpmSeconds); }

private:
	int mBeat =0;

	float mSeconds = 0.0f;
	float mBonusTime = 0.2f;
	std::vector<EvBuffRequest> mPendingBuffRequests;
};
