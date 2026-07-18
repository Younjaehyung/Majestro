#pragma once

#include <array>
#include "Protocol/Packet.h"

// 장면 전환을 넘어 유지되는 Majestro 게임 전용 상태
class MajestroGameInstance
{
public:
    static MajestroGameInstance& GetInstance()
    {
        static MajestroGameInstance instance;
        return instance;
    }

    void SetLocalRhythmVariantSelection(const RhythmVariantSelection& selection)
    {
        mLocalRhythmVariantSelection = SanitizeRhythmVariantSelection(
            static_cast<std::uint8_t>(selection.r1),
            static_cast<std::uint8_t>(selection.r2),
            static_cast<std::uint8_t>(selection.r3));
    }

    void SetLocalRhythmSubVariant(Rhythm rhythm, RhythmSubVariant selection)
    {
        mLocalRhythmVariantSelection.SetForRhythm(rhythm, selection);
    }

    const RhythmVariantSelection& GetLocalRhythmVariantSelection() const
    {
        return mLocalRhythmVariantSelection;
    }

    void ClearConfirmedRhythmVariantSelections()
    {
        mConfirmedRhythmVariantSelections.fill(RhythmVariantSelection{});
    }

    void SetConfirmedRhythmVariantSelectionForPlayerType(
        std::uint8_t playerType,
        const RhythmVariantSelection& selection)
    {
        if (playerType >= mConfirmedRhythmVariantSelections.size())
            return;

        mConfirmedRhythmVariantSelections[playerType] =
            SanitizeRhythmVariantSelection(
                static_cast<std::uint8_t>(selection.r1),
                static_cast<std::uint8_t>(selection.r2),
                static_cast<std::uint8_t>(selection.r3));
    }

    const RhythmVariantSelection& GetConfirmedRhythmVariantSelectionForPlayerType(
        std::uint8_t playerType) const
    {
        static const RhythmVariantSelection kDefaultSelection{};
        if (playerType >= mConfirmedRhythmVariantSelections.size())
            return kDefaultSelection;

        return mConfirmedRhythmVariantSelections[playerType];
    }


    void SetRoomHost(bool isHost) { mIsRoomHost = isHost; }
    bool IsRoomHost() const { return mIsRoomHost; }

private:
    MajestroGameInstance() = default;

    RhythmVariantSelection mLocalRhythmVariantSelection{};
    std::array<RhythmVariantSelection, ROOM_CHARACTER_COUNT>
        mConfirmedRhythmVariantSelections{};
    bool mIsRoomHost = false;
};
