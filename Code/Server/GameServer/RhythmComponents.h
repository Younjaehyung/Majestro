#pragma once
#include "Component.h"
#include "Protocol/RhythmDefinitions.h"


// 추가 효과
struct RhythmVariantEffectModifiers
{
    int32 maxHealthBonus = 0;
    float moveSpeedMultiplier = 1.0f;
    int32 attackPowerBonus = 0;
    float incomingHealingMultiplier = 1.0f;
    int32 temporaryShieldBonus = 0;
    float outgoingRhythmEffectMultiplier = 1.0f;
    float rhythmEffectRangeMultiplier = 1.0f;
    float healPackHealingMultiplier = 1.0f;
    float conquestProgressMultiplier = 1.0f;
};


class RhythmStateComponent : public Component<RhythmStateComponent>
{
public:
    Rhythm GetCurrentRhythm() const { return mCurrentRhythm; }
    Rhythm GetPendingRhythm() const { return mPendingRhythm; }
    RhythmSubVariant GetCurrentSubVariant() const
    {
        return mVariantSelection.GetForRhythm(mCurrentRhythm);
    }
    const RhythmVariantSelection& GetVariantSelection() const
    {
        return mVariantSelection;
    }
    bool HasPendingChange() const { return mHasPendingChange; }
    std::int64_t GetPendingApplyBeat() const { return mPendingApplyBeat; }

    void SetVariantSelection(const RhythmVariantSelection& selection)
    {
        mVariantSelection = SanitizeRhythmVariantSelection(
            static_cast<std::uint8_t>(selection.r1),
            static_cast<std::uint8_t>(selection.r2),
            static_cast<std::uint8_t>(selection.r3));
    }

    void ScheduleRhythmChange(Rhythm rhythm, std::int64_t applyBeat)
    {
        mPendingRhythm = SanitizeRhythm(ToRhythmValue(rhythm));
        mPendingApplyBeat = applyBeat;
        mHasPendingChange = true;
    }

    void ReplacePendingRhythm(Rhythm rhythm)
    {
        if (!mHasPendingChange)
            return;

        // 예약 음악만 교체하고 최초 적용 박자는 그대로 유지한다.
        mPendingRhythm = SanitizeRhythm(ToRhythmValue(rhythm));
    }

    void CancelPendingChange()
    {
        mPendingRhythm = mCurrentRhythm;
        mPendingApplyBeat = kNoRhythmApplyBeat;
        mHasPendingChange = false;
    }

    bool ApplyPendingChange(std::int64_t currentBeat)
    {
        if (!mHasPendingChange || currentBeat < mPendingApplyBeat)
            return false;

        mCurrentRhythm = mPendingRhythm;
        CancelPendingChange();
        return true;
    }

    Rhythm ForceNeutral()
    {
        const Rhythm previous = mCurrentRhythm;
        mCurrentRhythm = Rhythm::Neutral;
        CancelPendingChange();
        return previous;
    }

private:
    Rhythm mCurrentRhythm = Rhythm::Neutral;
    Rhythm mPendingRhythm = Rhythm::Neutral;
    RhythmVariantSelection mVariantSelection{};
    bool mHasPendingChange = false;
    std::int64_t mPendingApplyBeat = kNoRhythmApplyBeat;
};


class RhythmEffectComponent : public Component<RhythmEffectComponent>
{
public:
    const RhythmVariantEffectModifiers& GetVariantModifiers() const
    {
        return mVariantModifiers;
    }

    void SetVariantModifiers(const RhythmVariantEffectModifiers& modifiers)
    {
        mVariantModifiers = modifiers;
    }

    std::int32_t GetGrantedShieldBonus() const { return mGrantedShieldBonus; }
    void SetGrantedShieldBonus(std::int32_t bonus) { mGrantedShieldBonus = bonus; }

    std::int32_t GetRemainingTemporaryShield() const { return mRemainingTemporaryShield; }
    void AddTemporaryShield(std::int32_t amount)
    {
        mRemainingTemporaryShield = (std::max)(0, mRemainingTemporaryShield + amount);
    }
    void ConsumeTemporaryShield(std::int32_t amount)
    {
        mRemainingTemporaryShield = (std::max)(
            0, mRemainingTemporaryShield - (std::max)(0, amount));
    }
    void ClearTemporaryShield() { mRemainingTemporaryShield = 0; }

    bool IsBaseEffectProvisionActive(float currentTime) const
    {
        return currentTime < mBaseEffectProvideUntil;
    }

    void ExtendBaseEffectProvisionUntil(float endTime)
    {
        mBaseEffectProvideUntil = (std::max)(mBaseEffectProvideUntil, endTime);
    }

    void StopBaseEffectProvision() { mBaseEffectProvideUntil = 0.0f; }

private:
    RhythmVariantEffectModifiers mVariantModifiers{};
    int32 mGrantedShieldBonus = 0;
    int32 mRemainingTemporaryShield = 0;
    float mBaseEffectProvideUntil = 0.0f;
};
