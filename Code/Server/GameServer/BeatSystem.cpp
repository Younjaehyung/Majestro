#include "BeatSystem.h"
#include "pch.h"
#include "BeatSystem.h"
#include "BeatComponent.h"

#include "PlayerComponent.h"
#include "ArmorComponent.h"
#include "EventManager.h"
#include "GameEvents.h"
#include "BuffComponent.h"
#include "TransformComponent.h"
#include "GameTimer.h"
#include "HealthComponent.h"
#include "RhythmComponents.h"

BeatSystem::BeatSystem(World* world) : System(world)
{
}


void BeatSystem::Initialize()
{
}

void BeatSystem::Update(float dt)
{
	if (false == mWorld->HasComponentPool<BeatComponent>())return;

	CollectPendingBuffRequests();

	mSeconds += dt;
	//cout << "time :" << mSeconds << endl;

	mBeat = static_cast<int>(GetAbsoluteBeatIndex() % kMusicLoopBeatCount);
	//cout << "Beat :" << mBeat << endl;


	std::vector<Entity> entitys{ mWorld->GetEntitiesWithComponent<BeatComponent>() };

	for (auto& entity : entitys) {
		BeatComponent* beatComponent = mWorld->GetComponent<BeatComponent>(entity);
		beatComponent->mBeat = this->mBeat;

		if (RhythmStateComponent* rhythmState =
			mWorld->GetComponent<RhythmStateComponent>(entity))
		{
			// BeatSystem은 예약된 부모 리듬을 박자에 맞춰 확정하는 책임만 가짐
			if (rhythmState->ApplyPendingChange(GetAbsoluteBeatIndex()))
			{
				cout << "Rhythm Changed @beat " << GetAbsoluteBeatIndex()
					<< " : " << static_cast<int>(
						ToRhythmValue(rhythmState->GetCurrentRhythm())) << endl;
			}
		}
	}

	
	ApplyPendingBuffRequests();
}


float BeatSystem::GetBeatProgress() const
{
	// 현재 박자 안에서의 위치 (0=박자 시작, 1에 근접=다음 박자 직전)
	if (mBpmSeconds <= 0.f)
		return 0.f;
	const float progress = std::fmod(mSeconds, mBpmSeconds) / mBpmSeconds;
	return progress < 0.f ? progress + 1.f : progress;  // 곡 시작 전(음수 곡위치) 대비
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
		if (MainPlayerComponent* player = mWorld->GetComponent<MainPlayerComponent>(request.target))
		{
			if (player->IsDeathActive())
				continue;

			if (HealthComponent* health = mWorld->GetComponent<HealthComponent>(request.target))
			{
				if (health->mCurrentHp <= 0)
					continue;
			}
		}

		ArmorComponent* armorComponent = mWorld->GetComponent<ArmorComponent>(request.target);
		if (armorComponent == nullptr)
			continue;

		BuffComponent* buffComponent = mWorld->GetComponent<BuffComponent>(request.target);
		if (buffComponent == nullptr)
			continue;

		const float now = GetServerTotalTimeSeconds();

		if (request.skillType == SkillType::DrumSkill2)
		{
				armorComponent->mCurrentArmor = (std::min)(armorComponent->mCurrentArmor + 150, armorComponent->mMaxArmor);

			eventManager->Enqueue<EvArmorChanged>({ request.target, armorComponent->mCurrentArmor, armorComponent->mMaxArmor });

			BuffData buff;
			buff.mKind = EffectKind::Debuff;
			buff.mType = BuffType::ShieldDown;
			buff.mDurationPolicy = DurationPolicy::Timed;
			buff.mExecutionType = BuffExecutionType::Periodic;
				buff.mEndTime = now + mBpmSeconds * 15;

			buff.mTickInterval = mBpmSeconds;
			buff.mNextTriggerTime = now + mBpmSeconds;

			buffComponent->AddOrRefresh(buff);

			TransformComponent* transformComponent = mWorld->GetComponent<TransformComponent>(request.target);
			if (transformComponent != nullptr)
			{
				eventManager->Enqueue<EvEffectSpawn>(EvEffectSpawn{
					static_cast<uint8>(request.skillType),
					transformComponent->mWorldPosition.x,
					transformComponent->mWorldPosition.y,
					transformComponent->mWorldPosition.z,
					EffectSpawnReason::Fire,
					transformComponent->mLocalRotationE.x,
					transformComponent->mLocalRotationE.y,
					transformComponent->mLocalRotationE.z });
			}

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

			// DrumSkill3 vfx
			TransformComponent* transformComponent = mWorld->GetComponent<TransformComponent>(request.target);
			if (transformComponent != nullptr)
			{
				eventManager->Enqueue<EvEffectSpawn>(EvEffectSpawn{
					static_cast<uint8>(request.skillType),
					transformComponent->mWorldPosition.x,
					transformComponent->mWorldPosition.y,
					transformComponent->mWorldPosition.z,
					EffectSpawnReason::Fire,
					transformComponent->mLocalRotationE.x,
					transformComponent->mLocalRotationE.y,
					transformComponent->mLocalRotationE.z
					});
			}


		}
	}

	mPendingBuffRequests.clear();
}
