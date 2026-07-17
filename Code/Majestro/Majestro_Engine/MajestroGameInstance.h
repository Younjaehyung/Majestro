#pragma once

#include <array>
#include "Protocol/RhythmMusicDefinitions.h"

// 장면 전환을 넘어 유지되는 Majestro 게임 전용 상태
class MajestroGameInstance
{
public:
    static MajestroGameInstance& GetInstance()
    {
        static MajestroGameInstance instance;
        return instance;
    }

    void SetLocalRhythmMusicSelection(RhythmMusicVariant selection)
    {
        mLocalRhythmMusicSelection = SanitizeRhythmMusicVariant(
            static_cast<std::uint8_t>(selection));
    }

    void SetLocalRhythmMusicSelection(std::uint8_t value)
    {
        SetLocalRhythmMusicSelection(SanitizeRhythmMusicVariant(value));
    }

    RhythmMusicVariant GetLocalRhythmMusicSelection() const
    {
        return mLocalRhythmMusicSelection;
    }

    void ClearConfirmedRhythmMusicSelections()
    {
        mConfirmedRhythmMusicSelections.fill(RhythmMusicVariant::Original);
    }

    void SetConfirmedRhythmMusicSelectionForPlayerType(
        std::uint8_t playerType,
        RhythmMusicVariant selection)
    {
        if (playerType >= mConfirmedRhythmMusicSelections.size())
            return;

        mConfirmedRhythmMusicSelections[playerType] = SanitizeRhythmMusicVariant(
            static_cast<std::uint8_t>(selection));
    }

    RhythmMusicVariant GetConfirmedRhythmMusicSelectionForPlayerType(
        std::uint8_t playerType) const
    {
        if (playerType >= mConfirmedRhythmMusicSelections.size())
            return RhythmMusicVariant::Original;

        return mConfirmedRhythmMusicSelections[playerType];
    }

private:
    MajestroGameInstance() = default;

    RhythmMusicVariant mLocalRhythmMusicSelection = RhythmMusicVariant::Original;
    std::array<RhythmMusicVariant, kRhythmMusicPlayerTypeCount>
        mConfirmedRhythmMusicSelections{};
};
