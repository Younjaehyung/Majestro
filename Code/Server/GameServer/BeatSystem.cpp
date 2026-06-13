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

namespace
{
	struct RhythmBuffDef
	{
		BuffType          type = BuffType::None;					// 버프 유형
		BuffExecutionType exec = BuffExecutionType::Persistent;		// 버프 실행 방식
	};

	constexpr int kPlayerTypeCount = PlayerType::Count;


	constexpr RhythmBuffDef kRhythmBuffTable[kPlayerTypeCount][static_cast<int>(Rhythm::Count)] =
	{
		/* Rudwig  */ {
			{ BuffType::None,        BuffExecutionType::Persistent }, // Neutral
			{ BuffType::AttackUp,    BuffExecutionType::Persistent }, // R1
			{ BuffType::ScoreBoost,  BuffExecutionType::Persistent }, // R2
			{ BuffType::MoveSpeedUp, BuffExecutionType::Persistent }, // R3
		},
		/* Ibanix  */ {
			{ BuffType::None,           BuffExecutionType::Persistent }, // Neutral
			{ BuffType::ScoreOverTime,  BuffExecutionType::Periodic   }, // R1
			{ BuffType::ShieldOverTime, BuffExecutionType::Periodic   }, // R2
			{ BuffType::HealOverTime,   BuffExecutionType::Periodic   }, // R3
		},
		/* Fanthor */ {
			{ BuffType::None, BuffExecutionType::Persistent }, // Neutral
			{ BuffType::None, BuffExecutionType::Persistent }, // R1 (없음)
			{ BuffType::None, BuffExecutionType::Persistent }, // R2 (없음)
			{ BuffType::None, BuffExecutionType::Persistent }, // R3 (없음)
		},
	};

	
	const RhythmBuffDef& LookupRhythmBuff(uint8 playerType, uint8 rhythm)
	{
		static constexpr RhythmBuffDef kNone{};
		if (playerType >= kPlayerTypeCount || rhythm >= static_cast<uint8>(Rhythm::Count))
			return kNone;	// 범위 밖은 None
		return kRhythmBuffTable[playerType][rhythm];
	}
}

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

	for (auto& entity : entitys) {
		BeatComponent* beatComponent = mWorld->GetComponent<BeatComponent>(entity);
		beatComponent->mBeat = this->mBeat;

		//rythm buff----------------------------------------------------------

		if (auto* mainPlayerComponent = mWorld->GetComponent<MainPlayerComponent>(entity))
		{
			// look-ahead: 예약된 절대 박자에 도달했을 때만 전환 확정
			if (mainPlayerComponent->mHasQueuedRhythmChange && GetAbsoluteBeatIndex() >= mainPlayerComponent->mRhythmApplyBeat)
			{
				BuffComponent* buffComponent = mWorld->GetComponent<BuffComponent>(entity);
				if (buffComponent == nullptr)
					continue;

				const uint8 playerType = static_cast<uint8>(mainPlayerComponent->mPlayerType);
				const RhythmBuffDef& newDef = LookupRhythmBuff(playerType, mainPlayerComponent->mNextRhythm);
				const RhythmBuffDef& oldDef = LookupRhythmBuff(playerType, mainPlayerComponent->mRhythm);

				BuffData buff;
				buff.mKind = EffectKind::Buff;
				buff.mType = newDef.type;
				buff.mDurationPolicy = DurationPolicy::UntilSignal;
				buff.mExecutionType = newDef.exec;
				if (newDef.exec == BuffExecutionType::Periodic)
				{
					buff.mTickInterval = mBpmSeconds;
					buff.mNextTriggerTime = GetServerTotalTimeSeconds() + mBpmSeconds;
				}

				std::vector<Entity> players = mWorld->GetEntitiesWithComponent<MainPlayerComponent>();

				for (Entity player : players)
				{
					BuffComponent* buffComp = mWorld->GetComponent<BuffComponent>(player);
					if (buffComp == nullptr)
						continue;

					if (newDef.type != BuffType::None)
						buffComp->AddOrRefresh(buff);

					if (oldDef.type != BuffType::None)
						buffComp->RemoveBuff(oldDef.type);
				}

				mainPlayerComponent->mRhythm = mainPlayerComponent->mNextRhythm;
				mainPlayerComponent->mHasQueuedRhythmChange = false;
				mainPlayerComponent->mRhythmApplyBeat = -1;

				cout << "Rhythm Changed @beat " << GetAbsoluteBeatIndex()
					<< " : " << (int)mainPlayerComponent->mRhythm << endl;
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