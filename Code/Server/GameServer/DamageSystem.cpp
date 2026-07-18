#include "pch.h"
#include "DamageSystem.h"

#include "World.h"
#include "EventManager.h"
#include "GameEvents.h"
#include "HealthComponent.h"
#include "ArmorComponent.h"
#include "EnemyComponent.h"
#include "PlayerComponent.h"
#include "GravityComponent.h"
#include "MovementComponent.h"
#include "BeatSystem.h"
#include "GameTimer.h"
#include "RhythmComponents.h"
#include "RhythmEffectSystem.h"


DamageSystem::DamageSystem(World* world) : System(world)
{
}

void DamageSystem::ApplyRudwigCriticalRhythmEffect(
    Entity instigator,
    EventManager& eventManager)
{
    const MainPlayerComponent* instigatorPlayer =
        mWorld->GetComponent<MainPlayerComponent>(instigator);
    const RhythmStateComponent* rhythmState =
        mWorld->GetComponent<RhythmStateComponent>(instigator);
    const RhythmEffectComponent* rhythmEffect =
        mWorld->GetComponent<RhythmEffectComponent>(instigator);

    if (!instigatorPlayer || instigatorPlayer->mPlayerType != PlayerType::Rudwig ||
        !rhythmState || !rhythmEffect)
    {
        return;
    }

    float beatSeconds = 0.0f;
    if (auto systemManager = mWorld->GetSystemManager())
    {
        if (const BeatSystem* beatSystem = systemManager->GetSystem<BeatSystem>())
            beatSeconds = beatSystem->mBpmSeconds;
    }

    if (rhythmState->GetCurrentRhythm() == Rhythm::R2)
    {
        for (Entity playerEntity : mWorld->GetEntitiesWithComponent<MainPlayerComponent>())
        {
            ArmorComponent* armor = mWorld->GetComponent<ArmorComponent>(playerEntity);
            if (!armor)
                continue;

            const int32 beforeArmor = armor->mCurrentArmor;
            const int32 shieldAmount = static_cast<int32>(std::lround(
                10.0f * rhythmEffect->GetVariantModifiers()
                    .outgoingRhythmEffectMultiplier));
            armor->mCurrentArmor = (std::min)(
                armor->mMaxArmor, armor->mCurrentArmor + shieldAmount);

            if (beforeArmor != armor->mCurrentArmor)
            {
                eventManager.Enqueue<EvArmorChanged>({
                    playerEntity, armor->mCurrentArmor, armor->mMaxArmor });
            }
        }
        return;
    }

    const float now = GetServerTotalTimeSeconds();
    const BuffType critBuffType =
        rhythmState->GetCurrentRhythm() == Rhythm::R3
        ? BuffType::CritMoveSpeedUp
        : BuffType::CritAttackUp;

    for (Entity playerEntity : mWorld->GetEntitiesWithComponent<MainPlayerComponent>())
    {
        BuffComponent* buffComponent = mWorld->GetComponent<BuffComponent>(playerEntity);
        if (!buffComponent)
            continue;

        BuffData buff{};
        buff.mKind = EffectKind::Buff;
        buff.mType = critBuffType;
        buff.mDurationPolicy = DurationPolicy::Timed;
        buff.mExecutionType = BuffExecutionType::Persistent;
        buff.mEndTime = now + 4.0f * beatSeconds;
        buff.mSource = instigator;
        buffComponent->AddOrRefresh(buff);
    }
}

void DamageSystem::Update(float deltaTime)
{
    (void)deltaTime;

    auto eventManager = mWorld->GetEventManager();
    if (!eventManager)
        return;

    eventManager->Consume<EvDamage>([&](const EvDamage& e)
    {
        if (!e.target.IsValid())
            return;

        HealthComponent* health = mWorld->GetComponent<HealthComponent>(e.target);
        if (!health)
            return;

        if (EnemyComponent* targetEnemy = mWorld->GetComponent<EnemyComponent>(e.target))
        {
            if (targetEnemy->mEnemyType == EnemyType::Slime && !e.isCritical)
                return;
        }

        MainPlayerComponent* player = mWorld->GetComponent<MainPlayerComponent>(e.target);
        if (player && player->IsDeathActive())
            return;
        if (player && player->mDash)
            return;

        const int32 appliedDamage = (std::max)(0, e.amount);
        const int32 beforeHp = health->mCurrentHp;

        int32 remainDamage = appliedDamage;
        int32 beforeArmor = 0;
        int32 afterArmor = 0;
        ArmorComponent* armor = mWorld->GetComponent<ArmorComponent>(e.target);
        if (armor)
        {
            beforeArmor = armor->mCurrentArmor;
            armor->mCurrentArmor = (std::max)(0, armor->mCurrentArmor - remainDamage);
            afterArmor = armor->mCurrentArmor;
            remainDamage = (std::max)(0, remainDamage - beforeArmor);

			if (beforeArmor > afterArmor)
			{
				const int32 absorbedDamage = beforeArmor - afterArmor;
				if (RhythmEffectComponent* rhythmEffect = mWorld->GetComponent<RhythmEffectComponent>(e.target))
				{
					rhythmEffect->ConsumeTemporaryShield(absorbedDamage);
				}
			}

            if (beforeArmor != afterArmor)
            {
                EvArmorChanged armorChanged{};
                armorChanged.target = e.target;
                armorChanged.currentArmor = afterArmor;
                armorChanged.maxArmor = armor->mMaxArmor;
                eventManager->Enqueue<EvArmorChanged>(armorChanged);
            }
        }

        health->mCurrentHp = (std::max)(0, health->mCurrentHp - remainDamage);
        const int32 afterHp = health->mCurrentHp;

        const bool killedNow = beforeHp > 0 && afterHp == 0;

        if (beforeHp != afterHp)
        {
            EvHealthChanged healthChanged{};
            healthChanged.target = e.target;
            healthChanged.currentHp = afterHp;
            healthChanged.maxHp = health->mMaxHp;
            healthChanged.instigator = e.instigator;
            healthChanged.isCritical = e.isCritical;
            eventManager->Enqueue<EvHealthChanged>(healthChanged);
        }

        const bool armorAbsorbed = (beforeArmor != afterArmor);
        const bool hpDamaged = (beforeHp != afterHp);

        // 적에게 데미지를 준 플레이어를 기여자로 기록 → 사망 시 막타를 제외한 기여자가 어시스트를 받는다.
        if ((armorAbsorbed || hpDamaged) && e.instigator.IsValid())
        {
            if (EnemyComponent* damagedEnemy = mWorld->GetComponent<EnemyComponent>(e.target))
            {
                if (mWorld->GetComponent<MainPlayerComponent>(e.instigator))
                {
                    const uint32 attackerSession = mWorld->GetSessionIDByEntity(e.instigator);
                    if (attackerSession != 0)
                        damagedEnemy->mDamageContributors.insert(attackerSession);
                }
            }
        }

        if (player && (armorAbsorbed || hpDamaged) &&
            player->mFsm.GetState() == S_Dance1)
        {
            player->mFsm.ChangeState(player, IdleState::Instance());
        }

        if (player && (armorAbsorbed || hpDamaged) && e.instigator.IsValid())
        {
            const EnemyComponent* instigatorEnemy =
                mWorld->GetComponent<EnemyComponent>(e.instigator);
            RhythmStateComponent* rhythmState =
                mWorld->GetComponent<RhythmStateComponent>(e.target);
            const bool wasHitBySlime =
                instigatorEnemy && instigatorEnemy->mEnemyType == EnemyType::Slime;

            if (wasHitBySlime && rhythmState)
            {
                const Rhythm previousRhythm = rhythmState->GetCurrentRhythm();
                const bool needsForceNeutral =
                    previousRhythm != Rhythm::Neutral || rhythmState->HasPendingChange();
                if (needsForceNeutral)
                {
                    int64 applyAtBeatIndex = 0;
                    if (auto systemManager = mWorld->GetSystemManager())
                    {
                        if (BeatSystem* beatSystem = systemManager->GetSystem<BeatSystem>())
                            applyAtBeatIndex = beatSystem->GetAbsoluteBeatIndex();
                    }

                    rhythmState->ForceNeutral();
                    if (RhythmEffectComponent* rhythmEffect =
                        mWorld->GetComponent<RhythmEffectComponent>(e.target))
                    {
                        rhythmEffect->StopBaseEffectProvision();
                    }

                    if (auto systemManager = mWorld->GetSystemManager())
                    {
                        if (RhythmEffectSystem* effectSystem =
                            systemManager->GetSystem<RhythmEffectSystem>())
                        {
                            effectSystem->ApplyCurrentRhythmEffects(e.target);
                        }
                    }

                    EvRhythmChanged rhythmChanged{};
                    rhythmChanged.player = e.target;
                    rhythmChanged.previousRhythm = ToRhythmValue(previousRhythm);
                    rhythmChanged.changedRhythm = ToRhythmValue(Rhythm::Neutral);
                    rhythmChanged.playerType = player->mPlayerType;
                    rhythmChanged.applyAtBeatIndex = applyAtBeatIndex;
                    eventManager->Enqueue<EvRhythmChanged>(rhythmChanged);
                }
            }
        }

        if (armorAbsorbed || hpDamaged)
        {
            if (EnemyComponent* enemy = mWorld->GetComponent<EnemyComponent>(e.target))
            {
                const bool isRudwigCriticalAttack =
                    e.instigator.IsValid() && e.isCritical &&
                    (e.skillType == SkillType::DrumAttack ||
                     e.skillType == SkillType::DrumAttack3);
                if (isRudwigCriticalAttack)
                    ApplyRudwigCriticalRhythmEffect(e.instigator, *eventManager);

                if (enemy->mEnemyType == EnemyType::Pianoman)
                {
                    float beatSeconds = 0.0f;
                    if (auto systemManager = mWorld->GetSystemManager())
                    {
                        if (auto* beatSystem = systemManager->GetSystem<BeatSystem>())
                            beatSeconds = beatSystem->mBpmSeconds;
                    }

                    const float nowSeconds = GetServerTotalTimeSeconds();
                    const bool wasRushing =
                        enemy->mAnimState == static_cast<uint8>(EnemyAnimState::Attack) ||
                        enemy->mPianoRushVfxPlayed;
                    enemy->mNextAttackTime = nowSeconds + beatSeconds * enemy->mAttackCool;
                    if (wasRushing)
                    {
                        enemy->mRushEndAnimEndTime = nowSeconds + enemy->mRushEndAnimTime;
                        enemy->mAnimState = static_cast<uint8>(EnemyAnimState::Idle);
                    }
                    else
                    {
                        enemy->mAnimState = static_cast<uint8>(EnemyAnimState::Idle);
                    }
                    enemy->mPianoRushVfxPlayed = false;
                }
            }
        }

        if (e.instigator.IsValid() && appliedDamage > 0 && (armorAbsorbed || hpDamaged))
        {
            EvHitConfirm hit{};
            hit.instigator = e.instigator;
            hit.target = e.target;
            hit.damage = appliedDamage;
            hit.isKill = health->IsDead();
            eventManager->Enqueue<EvHitConfirm>(hit);
        }

        std::cout << "[DamageSystem] target=" << e.target.GetID()
            << " instigator=" << e.instigator.GetID()
            << " amount=" << e.amount
            << " armor=" << beforeArmor << "->" << afterArmor
            << " hp=" << afterHp << "/" << health->mMaxHp
            << " (" << beforeHp << "->" << afterHp << ")" << std::endl;

        if (!health->IsDead()) //         if (!killedNow)
            return;

        if (EnemyComponent* enemy = mWorld->GetComponent<EnemyComponent>(e.target))
        {
            if (enemy->mEnemyType == EnemyType::Obelisk)
                return;

            // 처치 사실만 발행한다. 점수/어시스트 집계는 ScoreSystem이 전담.
            EvEnemyKilled killed{};
            killed.enemyType = enemy->mEnemyType;

            if (e.instigator.IsValid())
            {
                if (MainPlayerComponent* instigatorPlayer = mWorld->GetComponent<MainPlayerComponent>(e.instigator))
                {
                    killed.killerSession = mWorld->GetSessionIDByEntity(e.instigator);
                    killed.killerType = static_cast<uint8>(instigatorPlayer->mPlayerType);
                }
            }

            // 데미지 기여자 목록을 값으로 복사(적 엔티티는 곧 파괴될 수 있음).
            for (uint32 session : enemy->mDamageContributors)
            {
                if (killed.contributorCount >= ROOM_MAX_PLAYERS)
                    break;
                killed.contributors[killed.contributorCount++] = session;
            }

            eventManager->Enqueue<EvEnemyKilled>(killed);
        }

        if (player)
        {
            // 사망
            EvPlayerDeathRequest deathEvent{ e.target, static_cast<int8>(PlayerDeathCause::Health), 10.0f ,false};
            eventManager->Enqueue<EvPlayerDeathRequest>(deathEvent);

            return;
        }

        EvDespawn despawnEvent{};
        despawnEvent.target = e.target;
        eventManager->Enqueue<EvDespawn>(despawnEvent);
    });

    eventManager->Consume<EvDespawn>([&](const EvDespawn& e)
    {
        if (!e.target.IsValid())
            return;
    });

    eventManager->Consume<EvHeal>([&](const EvHeal& e)
    {
        if (!e.target.IsValid())
            return;

        HealthComponent* health = mWorld->GetComponent<HealthComponent>(e.target);
        if (!health)
            return;

        if (MainPlayerComponent* player = mWorld->GetComponent<MainPlayerComponent>(e.target))
        {
            if (player->IsDeathActive())
                return;
        }
        if (health->mCurrentHp <= 0)
            return;

        float healingMultiplier = 1.0f;

        if (const RhythmEffectComponent* rhythmEffect = mWorld->GetComponent<RhythmEffectComponent>(e.target))
        {
            // 회복량 증가는 효과를 선택한 플레이어가 받는 모든 체력 회복에 적용
			healingMultiplier = rhythmEffect->GetVariantModifiers().incomingHealingMultiplier;
        }

        const int32 appliedHeal = (std::max)(0, static_cast<int32>(std::lround(
            static_cast<float>(e.amount) * healingMultiplier)));
        if (appliedHeal == 0)
            return;

        const int32 beforeHp = health->mCurrentHp;
        health->mCurrentHp = (std::min)(health->mMaxHp, health->mCurrentHp + appliedHeal);
        const int32 afterHp = health->mCurrentHp;

        if (beforeHp != afterHp)
        {
            EvHealthChanged healthChanged{};
            healthChanged.target = e.target;
            healthChanged.currentHp = afterHp;
            healthChanged.maxHp = health->mMaxHp;
            eventManager->Enqueue<EvHealthChanged>(healthChanged);

            // 플레이어가 아군을 실제로 회복시켰다면 지원 행동으로 발행 → ScoreSystem이 어시스트 창에 반영.
            if (e.instigator.IsValid() && e.instigator != e.target &&
                mWorld->GetComponent<MainPlayerComponent>(e.instigator))
            {
                eventManager->Enqueue<EvSupportAction>(EvSupportAction{ e.instigator });
            }
        }
    });
}
