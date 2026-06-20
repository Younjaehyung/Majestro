#include "pch.h"
#include "DamageSystem.h"

#include "World.h"
#include "EventManager.h"
#include "GameEvents.h"
#include "HealthComponent.h"
#include "ArmorComponent.h"
#include "EnemyComponent.h"
#include "PlayerComponent.h"
#include "ScoreBoardComponent.h"
#include "GravityComponent.h"
#include "MovementComponent.h"
#include "BeatSystem.h"
#include "GameTimer.h"

DamageSystem::DamageSystem(World* world) : System(world)
{
    if (mWorld && mWorld->GetSingleton<ScoreBoardComponent>() == nullptr)
        mWorld->AddSingleton<ScoreBoardComponent>();
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

        if (player && (armorAbsorbed || hpDamaged) && e.instigator.IsValid())
        {
            if (EnemyComponent* instigatorEnemy = mWorld->GetComponent<EnemyComponent>(e.instigator))
            {
                if (instigatorEnemy->mEnemyType == EnemyType::Slime)
                {
                    const uint8 previousRhythm = player->mRhythm;
                    const bool hadQueuedRhythmChange = player->mHasQueuedRhythmChange;
                    const bool needForceRhythmZero = (previousRhythm != 0) || hadQueuedRhythmChange || (player->mNextRhythm != 0);

                    if (needForceRhythmZero)
                    {
                        int64 applyAtBeatIndex = 0;
                        BeatSystem* beatSystem = nullptr;
                        if (auto systemManager = mWorld->GetSystemManager())
                            beatSystem = systemManager->GetSystem<BeatSystem>();

                        if (beatSystem)
                            applyAtBeatIndex = beatSystem->GetAbsoluteBeatIndex();

                        player->mRhythm = 0;
                        player->mNextRhythm = 0;
                        player->mHasQueuedRhythmChange = false;
                        player->mRhythmApplyBeat = -1;
                        player->mRhythmBuffProvideUntil = 0.0f;

                        if (beatSystem)
                            beatSystem->SyncAllRhythmBuffsNow();

                        EvRhythmChanged rhythmChanged{};
                        rhythmChanged.player = e.target;
                        rhythmChanged.previousRhythm = previousRhythm;
                        rhythmChanged.changedRhythm = 0;
                        rhythmChanged.playerType = player->mPlayerType;
                        rhythmChanged.applyAtBeatIndex = applyAtBeatIndex;
                        eventManager->Enqueue<EvRhythmChanged>(rhythmChanged);
                    }
                }
            }
        }

        if (armorAbsorbed || hpDamaged)
        {
            if (EnemyComponent* enemy = mWorld->GetComponent<EnemyComponent>(e.target))
            {
                if (e.instigator.IsValid() &&
                    e.skillType == SkillType::DrumAttack &&
                    e.isCritical)
                {
                    if (MainPlayerComponent* instigatorPlayer = mWorld->GetComponent<MainPlayerComponent>(e.instigator))
                    {
                        if (instigatorPlayer->mPlayerType == PlayerType::Rudwig)
                        {
                            float beatSeconds = 0.0f;
                            if (auto systemManager = mWorld->GetSystemManager())
                            {
                                if (auto* beatSystem = systemManager->GetSystem<BeatSystem>())
                                    beatSeconds = beatSystem->mBpmSeconds;
                            }

                            if (instigatorPlayer->mRhythm == static_cast<uint8>(Rhythm::R2))
                            {
                                if (mWorld->HasComponentPool<MainPlayerComponent>())
                                {
                                    for (Entity playerEntity : mWorld->GetEntitiesWithComponent<MainPlayerComponent>())
                                    {
                                        ArmorComponent* armor = mWorld->GetComponent<ArmorComponent>(playerEntity);
                                        if (!armor)
                                            continue;

                                        const int32 beforeArmor = armor->mCurrentArmor;
                                        armor->mCurrentArmor = (std::min)(armor->mMaxArmor, armor->mCurrentArmor + 10);

                                        if (beforeArmor != armor->mCurrentArmor)
                                        {
                                            EvArmorChanged armorChanged{};
                                            armorChanged.target = playerEntity;
                                            armorChanged.currentArmor = armor->mCurrentArmor;
                                            armorChanged.maxArmor = armor->mMaxArmor;
                                            eventManager->Enqueue<EvArmorChanged>(armorChanged);
                                        }
                                    }
                                }
                            }
                            else
                            {
                                instigatorPlayer->mRhythmBuffProvideUntil = GetServerTotalTimeSeconds() + 4.0f * beatSeconds;
                            }
                        }
                    }
                }

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

        if (!health->IsDead())
            return;

        if (EnemyComponent* enemy = mWorld->GetComponent<EnemyComponent>(e.target))
        {
            if (enemy->mEnemyType == EnemyType::Obelisk)
                return;

            if (e.instigator.IsValid())
            {
                if (MainPlayerComponent* instigatorPlayer = mWorld->GetComponent<MainPlayerComponent>(e.instigator))
                {
                    if (ScoreBoardComponent* scoreBoard = mWorld->GetSingleton<ScoreBoardComponent>())
                    {
                        scoreBoard->RecordKill(
                            e.instigator,
                            mWorld->GetSessionIDByEntity(e.instigator),
                            static_cast<uint8>(instigatorPlayer->mPlayerType),
                            enemy->mEnemyType);
                    }
                }
            }
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

        const int32 appliedHeal = (std::max)(0, e.amount);
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
        }
    });
}
