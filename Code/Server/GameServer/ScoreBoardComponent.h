#pragma once
#include "pch.h"
#include "Component.h"
#include "Entity.h"
#include "EnemyComponent.h"

struct PlayerMonsterKillStat
{
    uint32 mSessionId = 0;
    uint8 mPlayerType = 0;
    int32 mTotalKills = 0;
    std::array<int32, static_cast<size_t>(EnemyType::Slime) + 1> mKillsByEnemyType{};

    void RecordKill(uint8 enemyType)
    {
        ++mTotalKills;

        const size_t index = static_cast<size_t>(enemyType);
        if (index < mKillsByEnemyType.size())
            ++mKillsByEnemyType[index];
    }
};

class ScoreBoardComponent : public Component<ScoreBoardComponent>
{
public:
    ScoreBoardComponent() = default;

    void RecordKill(Entity playerEntity, uint32 sessionId, uint8 playerType, uint8 enemyType)
    {
        if (!playerEntity.IsValid())
            return;

        PlayerMonsterKillStat& stat = mPlayerKillStats[playerEntity.GetID()];
        stat.mSessionId = sessionId;
        stat.mPlayerType = playerType;
        stat.RecordKill(enemyType);
    }

    const PlayerMonsterKillStat* Find(Entity playerEntity) const
    {
        auto it = mPlayerKillStats.find(playerEntity.GetID());
        if (it == mPlayerKillStats.end())
            return nullptr;

        return &it->second;
    }

public:
    std::unordered_map<EntityID, PlayerMonsterKillStat> mPlayerKillStats;
};
