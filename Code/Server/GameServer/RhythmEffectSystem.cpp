#include "pch.h"
#include "RhythmEffectSystem.h"

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


namespace
{
    constexpr std::array<std::array<RhythmVariantEffectModifiers, kRhythmSubVariantCount>,
        ToRhythmValue(Rhythm::Count)> kVariantEffectTable =
    {{
        {{ {}, {}, {} }},
        {{
            RhythmVariantEffectModifiers{ .maxHealthBonus = 50 },
            RhythmVariantEffectModifiers{ .moveSpeedMultiplier = 1.10f },
            RhythmVariantEffectModifiers{ .attackPowerBonus = 10 }
        }},
        {{
            RhythmVariantEffectModifiers{ .incomingHealingMultiplier = 1.10f },
            RhythmVariantEffectModifiers{ .temporaryShieldBonus = 30 },
            RhythmVariantEffectModifiers{ .outgoingRhythmEffectMultiplier = 1.10f }
        }},
        {{
            RhythmVariantEffectModifiers{ .rhythmEffectRangeMultiplier = 1.10f },
            RhythmVariantEffectModifiers{ .healPackHealingMultiplier = 1.10f },
            RhythmVariantEffectModifiers{ .conquestProgressMultiplier = 1.10f }
        }}
    }};

    constexpr RhythmVariantEffectModifiers ResolveVariantModifiers(
        Rhythm rhythm,
        RhythmSubVariant subVariant)
    {
        const Rhythm safeRhythm = SanitizeRhythm(ToRhythmValue(rhythm));
        const RhythmSubVariant safeSubVariant = SanitizeRhythmSubVariant(
            static_cast<std::uint8_t>(subVariant));
        return kVariantEffectTable[ToRhythmValue(safeRhythm)]
            [static_cast<std::uint8_t>(safeSubVariant)];
    }

    // 사진 기획표의 대표 조합을 컴파일 시점에 확인한다.
    static_assert(ResolveVariantModifiers(
        Rhythm::R1, RhythmSubVariant::Variation2).attackPowerBonus == 10);

    struct BaseRhythmEffectDefinition
    {
        BuffType type = BuffType::None;
        BuffExecutionType execution = BuffExecutionType::Persistent;
    };

    constexpr BaseRhythmEffectDefinition kBaseRhythmEffectTable
        [static_cast<uint8>(PlayerType::Count)][ToRhythmValue(Rhythm::Count)] =
    {
        {
            { BuffType::None, BuffExecutionType::Persistent },
            { BuffType::AttackUp, BuffExecutionType::Persistent },
            { BuffType::None, BuffExecutionType::Persistent },
            { BuffType::MoveSpeedUp, BuffExecutionType::Persistent }
        },
        {
            { BuffType::None, BuffExecutionType::Persistent },
            { BuffType::MoveSpeedUp10, BuffExecutionType::Persistent },
            { BuffType::ShieldOverTime, BuffExecutionType::Periodic },
            { BuffType::HealOverTime, BuffExecutionType::Periodic }
        },
        {
            { BuffType::None, BuffExecutionType::Persistent },
            { BuffType::None, BuffExecutionType::Persistent },
            { BuffType::None, BuffExecutionType::Persistent },
            { BuffType::None, BuffExecutionType::Persistent }
        }
    };

    const BaseRhythmEffectDefinition& FindBaseRhythmEffect(
        PlayerType playerType,
        Rhythm rhythm)
    {
        static constexpr BaseRhythmEffectDefinition kNoEffect{};
        if (playerType >= PlayerType::Count)
            return kNoEffect;

        return kBaseRhythmEffectTable[static_cast<uint8>(playerType)]
            [ToRhythmValue(SanitizeRhythm(ToRhythmValue(rhythm)))];
    }

    bool IsSilenced(World& world, Entity playerEntity)
    {
        const BuffComponent* buff = world.GetComponent<BuffComponent>(playerEntity);
        return buff && buff->FindBuff(BuffType::Silence) != nullptr;
    }
}

RhythmEffectSystem::RhythmEffectSystem(World* world)
    : System(world)
{
    mPhase = SysPhase::Sim;
}

void RhythmEffectSystem::Update(float deltaTime)
{
    (void)deltaTime;

    if (!mWorld || !mWorld->HasComponentPool<RhythmStateComponent>())
        return;

    for (Entity playerEntity : mWorld->GetEntitiesWithComponent<RhythmStateComponent>())
        ApplyCurrentRhythmEffects(playerEntity);
}

void RhythmEffectSystem::ApplyCurrentRhythmEffects(Entity playerEntity)
{
    MainPlayerComponent* player = mWorld->GetComponent<MainPlayerComponent>(playerEntity);
    RhythmStateComponent* rhythmState = mWorld->GetComponent<RhythmStateComponent>(playerEntity);
    RhythmEffectComponent* rhythmEffect =
        mWorld->GetComponent<RhythmEffectComponent>(playerEntity);
    if (!player || !rhythmState || !rhythmEffect)
        return;

    UpdateVariantModifiers(*rhythmState, *rhythmEffect);
    SyncVariantResourceStats(playerEntity, *rhythmEffect);
    SyncBaseRhythmEffectForProvider(playerEntity, *player, *rhythmState, *rhythmEffect);
}

void RhythmEffectSystem::UpdateVariantModifiers(
    RhythmStateComponent& rhythmState, RhythmEffectComponent& rhythmEffect)
{
    rhythmEffect.SetVariantModifiers(ResolveVariantModifiers(
        rhythmState.GetCurrentRhythm(),
        rhythmState.GetCurrentSubVariant()));
}

void RhythmEffectSystem::SyncVariantResourceStats(
    Entity playerEntity, RhythmEffectComponent& rhythmEffect)
{
    const RhythmVariantEffectModifiers& modifiers = rhythmEffect.GetVariantModifiers();

    // 최대 체력
    if (HealthComponent* health = mWorld->GetComponent<HealthComponent>(playerEntity))
    {
        const int32 newMaxHp = (std::max)(1, health->mBaseMaxHp + modifiers.maxHealthBonus);
        if (newMaxHp != health->mMaxHp)
        {
            health->mMaxHp = newMaxHp;
            health->mCurrentHp = (std::min)(health->mCurrentHp, health->mMaxHp);
            if (auto eventManager = mWorld->GetEventManager())
            {
                eventManager->Enqueue<EvHealthChanged>({
                    playerEntity, health->mCurrentHp, health->mMaxHp });
            }
        }
    }

    ArmorComponent* armor = mWorld->GetComponent<ArmorComponent>(playerEntity);
    if (!armor)
        return;


    armor->mMaxArmor = (std::max)(1, armor->mBaseMaxArmor + modifiers.temporaryShieldBonus);

    const int32 targetBonus = modifiers.temporaryShieldBonus;
    if (targetBonus == rhythmEffect.GetGrantedShieldBonus())
    {
        armor->mCurrentArmor = (std::min)(armor->mCurrentArmor, armor->mMaxArmor);
        return;
    }

    // 이전에 지급했던 방어막 중 아직 남은 만큼만 먼저 회수
    const int32 removableShield = (std::min)(
        armor->mCurrentArmor, rhythmEffect.GetRemainingTemporaryShield());
    armor->mCurrentArmor -= removableShield;
    rhythmEffect.ClearTemporaryShield();

    // 새 보너스가 있으면 그만큼 실제 방어막을 지급
    if (targetBonus > 0)
    {
        armor->mCurrentArmor = (std::min)(
            armor->mMaxArmor, armor->mCurrentArmor + targetBonus);
        rhythmEffect.AddTemporaryShield(targetBonus);
    }

    rhythmEffect.SetGrantedShieldBonus(targetBonus);
    if (auto eventManager = mWorld->GetEventManager())
    {
        eventManager->Enqueue<EvArmorChanged>({
            playerEntity, armor->mCurrentArmor, armor->mMaxArmor });
    }
}

void RhythmEffectSystem::SyncBaseRhythmEffectForProvider(
    Entity provider,
    MainPlayerComponent& providerPlayer,
    RhythmStateComponent& rhythmState,
    const RhythmEffectComponent& rhythmEffect)
{
    const BaseRhythmEffectDefinition& definition = FindBaseRhythmEffect(
        providerPlayer.mPlayerType,
        rhythmState.GetCurrentRhythm());

    const float now = GetServerTotalTimeSeconds();

    const bool rudwigWindowActive =
        providerPlayer.mPlayerType != PlayerType::Rudwig ||
        rhythmEffect.IsBaseEffectProvisionActive(now);

    const bool shouldEnable =
        !IsSilenced(*mWorld, provider) &&
        definition.type != BuffType::None &&
        rudwigWindowActive;

    const TransformComponent* providerTransform =
        mWorld->GetComponent<TransformComponent>(provider);

    const float effectRadius = providerPlayer.mRhythmEffectRadius *
        rhythmEffect.GetVariantModifiers().rhythmEffectRangeMultiplier;

    const float effectRadiusSquared = effectRadius * effectRadius;

    for (Entity target : mWorld->GetEntitiesWithComponent<MainPlayerComponent>())
    {
        BuffComponent* targetBuff = mWorld->GetComponent<BuffComponent>(target);
        if (!targetBuff)
            continue;

        bool isInRange = true;
        if (target != provider && effectRadius > 0.0f)
        {
            if (!providerTransform)
            {
                isInRange = false;
            }
            else
            {
                const TransformComponent* targetTransform =
                    mWorld->GetComponent<TransformComponent>(target);
                if (!targetTransform)
                {
                    isInRange = false;
                }
                else
                {
                    const Vec3 difference =
                        targetTransform->mWorldPosition - providerTransform->mWorldPosition;
                    isInRange = difference.LengthSquared() <= effectRadiusSquared;
                }
            }
        }

        const bool enableForTarget = shouldEnable && isInRange;
        BuffData* current = targetBuff->FindRhythmBuffFromSource(provider);
        const bool needsRemoval = current &&
            (!enableForTarget ||
             current->mType != definition.type ||
             current->mExecutionType != definition.execution);

        if (needsRemoval)
            targetBuff->RemoveBuff(current->mType, provider, true);

        if (!enableForTarget || targetBuff->FindRhythmBuffFromSource(provider))
            continue;

        BuffData buff{};
        buff.mKind = EffectKind::Buff;
        buff.mType = definition.type;
        buff.mDurationPolicy = DurationPolicy::UntilSignal;
        buff.mExecutionType = definition.execution;
        buff.mSource = provider;
        buff.mIsRhythmEffect = true;
        if (definition.execution == BuffExecutionType::Periodic)
        {
            buff.mTickInterval = kMusicBeatSeconds;
            buff.mNextTriggerTime = now + kMusicBeatSeconds;
        }

        targetBuff->AddOrRefresh(buff);
    }
}
