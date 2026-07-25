#include "pch.h"
#include "BeatSystem.h"
#include "BeatComponent.h"
#include "GameEvents.h"

BeatSystem::BeatSystem(World* world) : System(world)
{
}

void BeatSystem::Initialize()
{
}

float BeatSystem::GetBeatProgress() const
{
	// 현재 박자 안에서의 위치 (0=박자 시작, 1에 근접=다음 박자 직전)
	if (mBpmSeconds <= 0.f)
		return 0.f;
	const float progress = std::fmod(mSeconds, mBpmSeconds) / mBpmSeconds;
	return progress < 0.f ? progress + 1.f : progress;  // 곡 시작 전(음수 곡위치) 대비
}

void BeatSystem::Update(float dt)
{
	// 박자 시계
	mSeconds += dt;

	const int64 absBeat = static_cast<int64>(mSeconds / mBpmSeconds);

	mBeat = static_cast<int>(absBeat % kMusicLoopBeatCount);
	const float s = mSeconds - static_cast<float>(absBeat) * mBpmSeconds;

	if (false == mWorld->HasComponentPool<BeatComponent>()) return;

	std::vector<Entity> entitys{ mWorld->GetEntitiesWithComponent<BeatComponent>() };


	// 새 박자 시작 시 이벤트 발행 (0.2초 윈도우 안에서 한 번만)
	if (s * s < mBonusTime * mBonusTime && mBeat != mLastFiredBeat)
	{
		mWorld->GetEventManager()->Enqueue(EvBeat{ mBeat });
		mLastFiredBeat = mBeat;
	}

	//cout << "seconds :" << s << endl;
	for (auto& entity : entitys) {
		BeatComponent* beatComponent = mWorld->GetComponent<BeatComponent>(entity);
		beatComponent->mBeat = this->mBeat;
	}

}

void BeatSystem::SyncSongPosition(float serverSongPos)
{
	// 박자 시계(mSeconds)를 서버 곡위치로 보정
	mHasSynced = true;
	const float diff = serverSongPos - mSeconds;
	if (fabsf(diff) > mResyncSnapThreshold)
		mSeconds = serverSongPos;        // 큰 오차: 즉시 스냅
	else
		mSeconds += diff * mDriftCorrectGain; // 작은 오차: 서서히 수렴
}
