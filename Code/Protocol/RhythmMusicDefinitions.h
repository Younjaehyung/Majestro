#pragma once

#include <cstdint>


constexpr std::int32_t kMusicBpm = 128;
constexpr std::int64_t kBeatsPerBar = 16;
constexpr float kMusicBeatSeconds = 60.0f / static_cast<float>(kMusicBpm);
constexpr float kMusicLoopSeconds = kMusicBeatSeconds * static_cast<float>(kBeatsPerBar);

constexpr std::uint8_t kRhythmMusicPlayerTypeCount = 3;

// 파생 음악
enum class RhythmMusicVariant : std::uint8_t
{
    Original = 0,
    Variation1 = 1,
    Variation2 = 2
};

// 잘못된 네트워크 원본으로 복구
constexpr RhythmMusicVariant SanitizeRhythmMusicVariant(std::uint8_t value)
{
    if (value > static_cast<std::uint8_t>(RhythmMusicVariant::Variation2))
        return RhythmMusicVariant::Original;

    return static_cast<RhythmMusicVariant>(value);
}
