#include "pch.h"
#include "BeatSystem.h"
#include "BeatComponent.h"
#include "TimeUtils.h"
#include "PlayerComponent.h"
#include "ArmorComponent.h"
#include "EventManager.h"
#include "GameEvents.h"


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

	CollectPendingBuffRequests();

	const int previousBeat = mBeat;

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

		if (mBeat % 4 == 0 && mBeat != previousBeat) {
			if (auto* mainPlayerComponent = mWorld->GetComponent<MainPlayerComponent>(entity))
			{
				if (mainPlayerComponent->mHasQueuedRhythmChange)
				{
					mainPlayerComponent->mRhythm = mainPlayerComponent->mNextRhythm;
					mainPlayerComponent->mHasQueuedRhythmChange = false;

					if (mainPlayerComponent->mPlayerType == 0) {

					}
					if (mainPlayerComponent->mPlayerType == 1) {

					}
				}


			}
		}
	}

	
	ApplyPendingBuffRequests();
}

void BeatSystem::CollectPendingBuffRequests()
{
	auto eventManager = mWorld->GetEventManager();
	if (!eventManager)
		return;

	eventManager->Consume<EvBuffRequest>([this](const EvBuffRequest& request)
		{
			if (!request.target.IsValid())
				return;

			mPendingBuffRequests.push_back(request);
		});
}

void BeatSystem::ApplyPendingBuffRequests()
{
	auto eventManager = mWorld->GetEventManager();
	if (!eventManager || mPendingBuffRequests.empty())
		return;

	for (const EvBuffRequest& request : mPendingBuffRequests)
	{
		ArmorComponent* armorComponent = mWorld->GetComponent<ArmorComponent>(request.target);
		if (armorComponent == nullptr)
			continue;

		if (request.skillType == SkillType::DrumSkill2)
		{
			armorComponent->mMaxArmor += 160;
			armorComponent->mCurrentArmor = (std::min)(armorComponent->mCurrentArmor + 160, armorComponent->mMaxArmor);

			eventManager->Enqueue<EvArmorChanged>({ request.target, armorComponent->mCurrentArmor, armorComponent->mMaxArmor });
		}
		else if (request.skillType == SkillType::DrumSkill3)
		{
			
		}
	}

	mPendingBuffRequests.clear();
}