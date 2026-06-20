#pragma once
#include "pch.h"
#include "Component.h"
#include "Entity.h"
#include "EnemyComponent.h"

struct PlayerScoreStat
{
    uint32 mSessionId = 0;
    uint8 mPlayerType = 0;
    int32 mScore = 0;
    int32 mTotalKills = 0;
    std::array<int32, static_cast<size_t>(EnemyType::Brass) + 1> mKillsByEnemyType{};

    void RecordKill(uint8 enemyType, int32 score)
    {
        ++mTotalKills;
        mScore += score;

        const size_t index = static_cast<size_t>(enemyType);
        if (index < mKillsByEnemyType.size())
            ++mKillsByEnemyType[index];
    }
};

class ScoreBoardComponent : public Component<ScoreBoardComponent>
{
public:
    ScoreBoardComponent() = default;

    static constexpr int32 mMonsterKillScore = 100;

    int32 RecordKill(uint32 sessionId, uint8 playerType, uint8 enemyType)
    {
        if (sessionId == 0)
            return 0;

        // SessionId keeps the score stable even if the player entity is recreated.
        PlayerScoreStat& stat = mPlayerStats[sessionId];
        stat.mSessionId = sessionId;
        stat.mPlayerType = playerType;
        stat.RecordKill(enemyType, mMonsterKillScore);
        return mMonsterKillScore;
    }

    const PlayerScoreStat* Find(uint32 sessionId) const
    {
        auto it = mPlayerStats.find(sessionId);
        if (it == mPlayerStats.end())
            return nullptr;

        return &it->second;
    }

public:
    std::unordered_map<uint32, PlayerScoreStat> mPlayerStats;
};
