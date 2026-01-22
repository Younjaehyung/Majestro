#include "pch.h"
#include "BeatSystem.h"
#include "BeatComponent.h"

BeatSystem::BeatSystem(World* world) : System(world)
{
	mBpmSeconds = 60.f / (float)mBpm;
}

void BeatSystem::Initialize()
{
}

void BeatSystem::Update(float dt)
{

	mSeconds += dt;
	//cout << "time :" << mSeconds << endl;
	mBeat = (int)(mSeconds / mBpmSeconds);
	mBeat %= mBpm;
	//cout << "Beat :" << mBeat << endl;
	float s = mSeconds - (float)mBeat * mBpmSeconds;
	if (false == mWorld->HasComponentPool<BeatComponent>())return;
	std::vector<Entity> entitys{ mWorld->GetEntitiesWithComponent<BeatComponent>() };


	//cout << "seconds :" << s << endl;
	for (auto& entity : entitys) {
		BeatComponent* beatComponent = mWorld->GetComponent<BeatComponent>(entity);
		beatComponent->mBeat = this->mBeat;
		if (s*s < mBonusTime* mBonusTime)beatComponent->mBouns = true;
		else beatComponent->mBouns = false;
	}
	
}