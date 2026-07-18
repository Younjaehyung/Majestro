#pragma once

#include <cstdint>


constexpr std::int32_t kMusicBpm = 128;
constexpr std::int64_t kMusicLoopBeatCount = 16;
constexpr float kMusicBeatSeconds = 60.0f / static_cast<float>(kMusicBpm);
constexpr float kMusicLoopSeconds = kMusicBeatSeconds * static_cast<float>(kMusicLoopBeatCount);
constexpr std::uint8_t kRhythmSubVariantCount = 3;
constexpr std::int64_t kNoRhythmApplyBeat = -1;


enum class Rhythm : std::uint8_t
{
    Neutral = 0,
    R1,
    R2,
    R3,
    Count
};

// 하위 리듬
enum class RhythmSubVariant : std::uint8_t
{
    Original = 0,
    Variation1 = 1,
    Variation2 = 2
};

constexpr std::uint8_t ToRhythmValue(Rhythm rhythm)
{
    return static_cast<std::uint8_t>(rhythm);
}

constexpr Rhythm SanitizeRhythm(std::uint8_t value)
{
    if (value >= ToRhythmValue(Rhythm::Count))
        return Rhythm::Neutral;

    return static_cast<Rhythm>(value);
}

constexpr Rhythm GetNextRhythm(Rhythm rhythm)
{
    const Rhythm safeRhythm = SanitizeRhythm(ToRhythmValue(rhythm));
    const std::uint8_t next = static_cast<std::uint8_t>(
        (ToRhythmValue(safeRhythm) + 1) % ToRhythmValue(Rhythm::Count));
    return static_cast<Rhythm>(next);
}



// 잘못된 입력은 기본 하위 리듬으로 복구
constexpr RhythmSubVariant SanitizeRhythmSubVariant(std::uint8_t value)
{
    if (value >= kRhythmSubVariantCount)
        return RhythmSubVariant::Original;

    return static_cast<RhythmSubVariant>(value);
}

// 부모 리듬별로 게임 시작 전에 확정한 파생 음악과 추가 효과 선택값
struct RhythmVariantSelection
{
    RhythmSubVariant r1 = RhythmSubVariant::Original;
    RhythmSubVariant r2 = RhythmSubVariant::Original;
    RhythmSubVariant r3 = RhythmSubVariant::Original;

    constexpr RhythmSubVariant GetForRhythm(Rhythm rhythm) const
    {
        switch (SanitizeRhythm(ToRhythmValue(rhythm)))
        {
        case Rhythm::R1: return r1;
        case Rhythm::R2: return r2;
        case Rhythm::R3: return r3;
        default: return RhythmSubVariant::Original;
        }
    }

    void SetForRhythm(Rhythm rhythm, RhythmSubVariant subVariant)
    {
        const RhythmSubVariant safeSubVariant = SanitizeRhythmSubVariant(
            static_cast<std::uint8_t>(subVariant));

        switch (SanitizeRhythm(ToRhythmValue(rhythm)))
        {
        case Rhythm::R1: r1 = safeSubVariant; break;
        case Rhythm::R2: r2 = safeSubVariant; break;
        case Rhythm::R3: r3 = safeSubVariant; break;
        default: break;
        }
    }
};

constexpr RhythmVariantSelection SanitizeRhythmVariantSelection(
    std::uint8_t r1,
    std::uint8_t r2,
    std::uint8_t r3)
{
    return RhythmVariantSelection{
        SanitizeRhythmSubVariant(r1),
        SanitizeRhythmSubVariant(r2),
        SanitizeRhythmSubVariant(r3)
    };
}
