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
			{ BuffType::None,        BuffExecutionType::Persistent }, // R2 -> crit DrumAttack hit: team shield +10
			{ BuffType::MoveSpeedUp, BuffExecutionType::Persistent }, // R3
		},
		/* Ibanix  */ {
			{ BuffType::None,           BuffExecutionType::Persistent }, // Neutral
			{ BuffType::MoveSpeedUp10,  BuffExecutionType::Persistent }, // R1 -> 10% move speed
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

	uint8 GetEffectiveRhythmForBuffs(const MainPlayerComponent* providerPlayer)
	{
		if (!providerPlayer)
			return 0;

		return providerPlayer->mHasQueuedRhythmChange
			? providerPlayer->mNextRhythm
			: providerPlayer->mRhythm;
	}

	bool IsSilenced(World* world, Entity playerEntity)
	{
		if (!world || !playerEntity.IsValid())
			return false;

		BuffComponent* buffComponent = world->GetComponent<BuffComponent>(playerEntity);
		return buffComponent && buffComponent->FindBuff(BuffType::Silence) != nullptr;
	}

	void SyncRhythmBuffForProvider(World* world, Entity provider, MainPlayerComponent* providerPlayer, float bpmSeconds)
	{
		if (!world || !providerPlayer || !world->HasComponentPool<MainPlayerComponent>())
			return;

		const uint8 effectiveRhythm = GetEffectiveRhythmForBuffs(providerPlayer);
		const RhythmBuffDef& def = LookupRhythmBuff(static_cast<uint8>(providerPlayer->mPlayerType), effectiveRhythm);
		const float now = GetServerTotalTimeSeconds();
		const bool ludwigWindowActive =
			providerPlayer->mPlayerType != PlayerType::Rudwig ||
			now < providerPlayer->mRhythmBuffProvideUntil;
		const bool shouldEnable =
			!IsSilenced(world, provider) &&
			def.type != BuffType::None &&
			ludwigWindowActive;

		// 리듬 버프 범위
		const TransformComponent* providerTransform = world->GetComponent<TransformComponent>(provider);
		const float buffRadius = providerPlayer->mRhythmBuffRadius;
		const float buffRadiusSq = buffRadius * buffRadius;

		std::vector<Entity> players = world->GetEntitiesWithComponent<MainPlayerComponent>();
		for (Entity player : players)
		{
			BuffComponent* buffComp = world->GetComponent<BuffComponent>(player);
			if (!buffComp)
				continue;

			// 거리 검사
			bool withinRange = true;
			if (player != provider && buffRadius > 0.0f && providerTransform)
			{
				const TransformComponent* playerTransform = world->GetComponent<TransformComponent>(player);
				if (!playerTransform)
				{
					withinRange = false;
				}
				else
				{
					const Vec3 diff = playerTransform->mWorldPosition - providerTransform->mWorldPosition;
					withinRange = diff.LengthSquared() <= buffRadiusSq;
				}
			}

			const bool enableForThisPlayer = shouldEnable && withinRange;

			BuffData* current = buffComp->FindRhythmBuffFromSource(provider);
			const bool needsRemoval =
				current && (!enableForThisPlayer || current->mType != def.type || current->mExecutionType != def.exec);

			if (needsRemoval)
				buffComp->RemoveBuff(current->mType, provider, true);

			if (!enableForThisPlayer)
				continue;

			current = buffComp->FindRhythmBuffFromSource(provider);
			if (current)
				continue;

			BuffData buff;
			buff.mKind = EffectKind::Buff;
			buff.mType = def.type;
			buff.mDurationPolicy = DurationPolicy::UntilSignal;
			buff.mExecutionType = def.exec;
			buff.mSource = provider;
			buff.mIsRhythmEffect = true;
			if (def.exec == BuffExecutionType::Periodic)
			{
				buff.mTickInterval = bpmSeconds;
				buff.mNextTriggerTime = now + bpmSeconds;
			}

			buffComp->AddOrRefresh(buff);
		}
	}
}

BeatSystem::BeatSystem(World* world) : System(world)
{
}

void BeatSystem::Initialize()
{
}

void BeatSystem::SyncAllRhythmBuffsNow()
{
	if (!mWorld || !mWorld->HasComponentPool<MainPlayerComponent>())
		return;

	std::vector<Entity> players = mWorld->GetEntitiesWithComponent<MainPlayerComponent>();
	for (Entity entity : players)
	{
		if (auto* mainPlayerComponent = mWorld->GetComponent<MainPlayerComponent>(entity))
			SyncRhythmBuffForProvider(mWorld, entity, mainPlayerComponent, mBpmSeconds);
	}
}

void BeatSystem::Update(float dt)
{
	if (false == mWorld->HasComponentPool<BeatComponent>())return;

	CollectPendingBuffRequests();

	const int previousBeat = mBeat;

	mSeconds += dt;
	//cout << "time :" << mSeconds << endl;

	mBeat = static_cast<int>(GetAbsoluteBeatIndex() % kBeatsPerBar);
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
				mainPlayerComponent->mRhythm = mainPlayerComponent->mNextRhythm;
				mainPlayerComponent->mHasQueuedRhythmChange = false;
				mainPlayerComponent->mRhythmApplyBeat = -1;

				cout << "Rhythm Changed @beat " << GetAbsoluteBeatIndex()
					<< " : " << (int)mainPlayerComponent->mRhythm << endl;
			}


		}
		
	}

	SyncAllRhythmBuffsNow();

	
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
