#include "pch.h"
#include "BeatSystem.h"
#include "BeatComponent.h"
#include "TimeUtils.h"
#include "PlayerComponent.h"

BeatSystem::BeatSystem(World* world) : System(world)
{
	mBpmSeconds = 60.f / (float)mBpm;
	GetSteadyTimeSeconds();
}

void BeatSystem::Initialize()
{
}

void BeatSystem::Update(float dt)
{
	if (false == mWorld->HasComponentPool<BeatComponent>())return;

	mSeconds += dt;
	//cout << "time :" << mSeconds << endl;
	mBeat = (int)(mSeconds / mBpmSeconds);
	mBeat %= mBpm;
	//cout << "Beat :" << mBeat << endl;


	std::vector<Entity> entitys{ mWorld->GetEntitiesWithComponent<BeatComponent>() };

	float s = mSeconds - (float)mBeat * mBpmSeconds;
	//cout << "seconds :" << s << endl;
	for (auto& entity : entitys) {
		BeatComponent* beatComponent = mWorld->GetComponent<BeatComponent>(entity);
		beatComponent->mBeat = this->mBeat;
		if (s*s < mBonusTime* mBonusTime)beatComponent->mBouns = true;
		else beatComponent->mBouns = false;


		if (mBeat % 4 == 0) {
			if (auto* mainPlayerComponent = mWorld->GetComponent<MainPlayerComponent>(entity))
			{
				if (mainPlayerComponent->mHasQueuedRhythmChange)
				{
					mainPlayerComponent->mRhythm = mainPlayerComponent->mNextRhythm;
					mainPlayerComponent->mHasQueuedRhythmChange = false;
				}
			}
		}
	}
	
}