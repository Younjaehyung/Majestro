#include "pch.h"
#include "BuffSystem.h"

#include "ArmorComponent.h"
#include "BuffComponent.h"
#include "EventManager.h"
#include "GameEvents.h"
#include "GameTimer.h"
#include "HealthComponent.h"
#include "PlayerComponent.h"
#include "RhythmComponents.h"
#include "TransformComponent.h"
#include "World.h"


BuffSystem::BuffSystem(World* world) : System(world)
{
    mPhase = SysPhase::Sim;
}

void BuffSystem::Update(float dt)
{
    (void)dt;

    if (false == mWorld->HasComponentPool<BuffComponent>())
        return;

    const float now = GetServerTotalTimeSeconds();

    std::vector<Entity> entities = mWorld->GetEntitiesWithComponent<BuffComponent>();
    for (Entity entity : entities)
    {
        BuffComponent* buffComponent = mWorld->GetComponent<BuffComponent>(entity);
        if (buffComponent == nullptr)
            continue;

        size_t i = 0;
        while (i < buffComponent->mBuffs.size())
        {
            BuffData& buff = buffComponent->mBuffs[i];

            if (buff.mExecutionType == BuffExecutionType::Periodic && buff.mTickInterval > 0.0f)
            {
                while (now >= buff.mNextTriggerTime)
                {
                    ExecutePeriodicBuff(entity, buff);
                    buff.mNextTriggerTime += buff.mTickInterval;
                }
            }

            const bool isTimedOut = (buff.mDurationPolicy == DurationPolicy::Timed) && (now >= buff.mEndTime);
            if (isTimedOut)
            {
                buffComponent->RemoveBuff(buff.mType, buff.mSource, buff.mIsRhythmEffect);
                continue;
            }

            ++i;
        }
    }
}

void BuffSystem::ExecutePeriodicBuff(Entity target, BuffData& buff)
{
    auto eventManager = mWorld->GetEventManager();
    if (!eventManager)
        return;

    if (MainPlayerComponent* player = mWorld->GetComponent<MainPlayerComponent>(target))
    {
        if (player->IsDeathActive())
            return;

        if (HealthComponent* health = mWorld->GetComponent<HealthComponent>(target))
        {
            if (health->mCurrentHp <= 0)
                return;
        }
    }

    float sourceEffectMultiplier = 1.0f;
    if (buff.mSource.IsValid())
    {
        if (const RhythmEffectComponent* sourceEffect =
			mWorld->GetComponent<RhythmEffectComponent>(buff.mSource))
        {
			sourceEffectMultiplier = sourceEffect->GetVariantModifiers().outgoingRhythmEffectMultiplier;
        }
    }

    auto consumeScaledPeriodicAmount = [&buff, sourceEffectMultiplier](float baseAmount)
    {
        const float accumulated = baseAmount * sourceEffectMultiplier + buff.mFractionalRemainder;
        const int32 amount = static_cast<int32>(std::floor(accumulated));
        buff.mFractionalRemainder = accumulated - static_cast<float>(amount);
        return amount;
    };

    switch (buff.mType)
    {
    case BuffType::ShieldDown:
    {
        ArmorComponent* armor = mWorld->GetComponent<ArmorComponent>(target);
        if (!armor)
            return;

        const int32 beforeArmor = armor->mCurrentArmor;
        armor->mCurrentArmor = (std::max)(0, armor->mCurrentArmor - 10);
		if (RhythmEffectComponent* rhythmEffect =
			mWorld->GetComponent<RhythmEffectComponent>(target))
		{
			const int32 removedShield = beforeArmor - armor->mCurrentArmor;
			rhythmEffect->ConsumeTemporaryShield(removedShield);
		}

        if (beforeArmor != armor->mCurrentArmor)
        {
            eventManager->Enqueue<EvArmorChanged>({ target, armor->mCurrentArmor, armor->mMaxArmor });
        }
        break;
    }
    case BuffType::HealOverTime:
    {
        HealthComponent* health = mWorld->GetComponent<HealthComponent>(target);
        if (!health)
            return;

        const int32 beforeHp = health->mCurrentHp;
        float receivedHealingMultiplier = 1.0f;
        if (const RhythmEffectComponent* targetEffect =
			mWorld->GetComponent<RhythmEffectComponent>(target))
        {
			receivedHealingMultiplier =
				targetEffect->GetVariantModifiers().incomingHealingMultiplier;
        }
        const int32 healAmount = consumeScaledPeriodicAmount(
            5.0f * receivedHealingMultiplier);
        health->mCurrentHp = (std::min)(health->mMaxHp, health->mCurrentHp + healAmount);

        if (beforeHp != health->mCurrentHp)
        {
            eventManager->Enqueue<EvHealthChanged>({ target, health->mCurrentHp, health->mMaxHp });
        }
        break;
    }
    case BuffType::ShieldOverTime:
    {
        ArmorComponent* armor = mWorld->GetComponent<ArmorComponent>(target);
        if (!armor)
            return;

        const int32 beforeArmor = armor->mCurrentArmor;
        const int32 shieldAmount = consumeScaledPeriodicAmount(2.0f);
        armor->mCurrentArmor = (std::min)(armor->mMaxArmor, armor->mCurrentArmor + shieldAmount);

        if (beforeArmor != armor->mCurrentArmor)
        {
            eventManager->Enqueue<EvArmorChanged>({ target, armor->mCurrentArmor, armor->mMaxArmor });
        }
        break;
    }
    case BuffType::MoveSpeedUp10:
    default:
        break;
    }
}
