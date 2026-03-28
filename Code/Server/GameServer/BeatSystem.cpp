#include "pch.h"
#include "BeatSystem.h"
#include "BeatComponent.h"

#include "PlayerComponent.h"
#include "ArmorComponent.h"
#include "EventManager.h"
#include "GameEvents.h"
#include "BuffComponent.h"
#include "GameTimer.h"

BeatSystem::BeatSystem(World* world) : System(world)
{
	mBpmSeconds = 60.f / (float)mBpm;
	
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
					BuffComponent* buffComponent = mWorld->GetComponent<BuffComponent>(entity);
					if (buffComponent == nullptr)
						continue;
					BuffData buff;
					BuffType rBuff = BuffType::None;

					if (mainPlayerComponent->mPlayerType == 0) {
						switch (mainPlayerComponent->mNextRhythm) {
						case 1:
							buff.mKind = EffectKind::Buff;
							buff.mType = BuffType::AttackUp;
							buff.mDurationPolicy = DurationPolicy::UntilSignal;
							buff.mExecutionType = BuffExecutionType::Persistent;
							break;
						case 2:
							buff.mKind = EffectKind::Buff;
							buff.mType = BuffType::ScoreBoost;
							buff.mDurationPolicy = DurationPolicy::UntilSignal;
							buff.mExecutionType = BuffExecutionType::Persistent;
							break;
						case 3:
							buff.mKind = EffectKind::Buff;
							buff.mType = BuffType::MoveSpeedUp;
							buff.mDurationPolicy = DurationPolicy::UntilSignal;
							buff.mExecutionType = BuffExecutionType::Persistent;
							break;
						}
						switch (mainPlayerComponent->mRhythm) {
						case 1:
							rBuff = BuffType::AttackUp;
							break;
						case 2:
							rBuff = BuffType::ScoreBoost;
							break;
						case 3:
							rBuff = BuffType::MoveSpeedUp;
							break;
						}
					}
					if (mainPlayerComponent->mPlayerType == 1) {
						switch (mainPlayerComponent->mNextRhythm) {
						case 1:
							buff.mKind = EffectKind::Buff;
							buff.mType = BuffType::ScoreOverTime;
							buff.mDurationPolicy = DurationPolicy::UntilSignal;
							buff.mExecutionType = BuffExecutionType::Periodic;
							buff.mTickInterval = mBpmSeconds;
							buff.mNextTriggerTime = GetServerTotalTimeSeconds() + mBpmSeconds;
							break;
						case 2:
							buff.mKind = EffectKind::Buff;
							buff.mType = BuffType::ShieldOverTime;
							buff.mDurationPolicy = DurationPolicy::UntilSignal;
							buff.mExecutionType = BuffExecutionType::Periodic;
							buff.mTickInterval = mBpmSeconds;
							buff.mNextTriggerTime = GetServerTotalTimeSeconds() + mBpmSeconds;
							break;
						case 3:
							buff.mKind = EffectKind::Buff;
							buff.mType = BuffType::HealOverTime;
							buff.mDurationPolicy = DurationPolicy::UntilSignal;
							buff.mExecutionType = BuffExecutionType::Periodic;
							buff.mTickInterval = mBpmSeconds;
							buff.mNextTriggerTime = GetServerTotalTimeSeconds() + mBpmSeconds;
							break;
						}
						switch (mainPlayerComponent->mRhythm) {
						case 1:
							rBuff = BuffType::ScoreOverTime;
							break;
						case 2:
							rBuff = BuffType::ShieldOverTime;
							break;
						case 3:
							rBuff = BuffType::HealOverTime;
							break;
						}
					}

					std::vector<Entity> players = mWorld->GetEntitiesWithComponent<MainPlayerComponent>();
					for (Entity player : players)
					{
						BuffComponent* buffComp = mWorld->GetComponent<BuffComponent>(player);
						if (buffComp == nullptr)
							continue;

						if (mainPlayerComponent->mNextRhythm != 0)
							buffComp->AddOrRefresh(buff);

						if (mainPlayerComponent->mRhythm != 0)
							buffComp->RemoveBuff(rBuff);
						
					}

					mainPlayerComponent->mRhythm = mainPlayerComponent->mNextRhythm;
					mainPlayerComponent->mHasQueuedRhythmChange = false;

					cout << "Rhythm Changed:" << (int)mainPlayerComponent->mRhythm << endl;
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

		BuffComponent* buffComponent = mWorld->GetComponent<BuffComponent>(request.target);
		if (buffComponent == nullptr)
			continue;

		const float now = GetServerTotalTimeSeconds();

		if (request.skillType == SkillType::DrumSkill2)
		{
			armorComponent->mMaxArmor += 160;
			armorComponent->mCurrentArmor = (std::min)(armorComponent->mCurrentArmor + 160, armorComponent->mMaxArmor);

			eventManager->Enqueue<EvArmorChanged>({ request.target, armorComponent->mCurrentArmor, armorComponent->mMaxArmor });

			BuffData buff;
			buff.mKind = EffectKind::Debuff;
			buff.mType = BuffType::ShieldDown;
			buff.mDurationPolicy = DurationPolicy::Timed;
			buff.mExecutionType = BuffExecutionType::Periodic;
			buff.mEndTime = now + mBpmSeconds *16;

			buff.mTickInterval = mBpmSeconds;
			buff.mNextTriggerTime = now + mBpmSeconds;

			buffComponent->AddOrRefresh(buff);

		}
		else if (request.skillType == SkillType::DrumSkill3)
		{

			BuffData buff;
			buff.mKind = EffectKind::Buff;
			buff.mType = BuffType::BuffPowerUp;
			buff.mDurationPolicy = DurationPolicy::Timed;
			buff.mExecutionType = BuffExecutionType::Persistent;
			buff.mEndTime = now + mBpmSeconds *8;

			std::vector<Entity> players = mWorld->GetEntitiesWithComponent<MainPlayerComponent>();
			for (Entity player : players)
			{
				BuffComponent* buffComp = mWorld->GetComponent<BuffComponent>(player);
				if (buffComp == nullptr)
					continue;

				buffComp->AddOrRefresh(buff);
			}
		}
	}

	mPendingBuffRequests.clear();
}