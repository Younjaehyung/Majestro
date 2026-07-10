#pragma once
#include "pch.h"
#include "Component.h"
#include "Entity.h"
#include "EnemyComponent.h"

struct PlayerScoreStat
{
    uint32 mSessionId = 0;
    uint8 mPlayerType = 0;
    int32 mScore = 0;        // 처치 점수 누적
    int32 mTotalKills = 0;   // 막타로 처치한 적 수
    int32 mAssists = 0;      // 처치 기여(Deal/Heal/Buff)했지만 막타는 아닌 수
    int32 mMaxCombo = 0;     // 씬 내 최대 콤보
    float mLastSupportTime = -1000.0f; // 마지막으로 아군을 힐/버프한 서버 시각(초). 어시스트 창 판정용.
    std::array<int32, static_cast<size_t>(EnemyType::Dragon) + 1> mKillsByEnemyType{};

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
    // 힐/버프 지원 후 이 시간(초) 안에 적이 죽으면 지원 플레이어에게 어시스트를 준다.
    static constexpr float kSupportAssistWindow = 5.0f;

    // 통계 항목을 보장(없으면 생성)하고 플레이어 타입을 갱신한다.
    PlayerScoreStat& Ensure(uint32 sessionId, uint8 playerType)
    {
        PlayerScoreStat& stat = mPlayerStats[sessionId];
        stat.mSessionId = sessionId;
        stat.mPlayerType = playerType;
        return stat;
    }

    int32 RecordKill(uint32 sessionId, uint8 playerType, uint8 enemyType)
    {
        if (sessionId == 0)
            return 0;


        PlayerScoreStat& stat = Ensure(sessionId, playerType);
        stat.RecordKill(enemyType, mMonsterKillScore);
        return mMonsterKillScore;
    }

    void AddAssist(uint32 sessionId)
    {
        if (sessionId == 0)
            return;

        auto it = mPlayerStats.find(sessionId);
        if (it != mPlayerStats.end())
            ++it->second.mAssists;
    }

    void UpdateMaxCombo(uint32 sessionId, uint8 playerType, int32 combo)
    {
        if (sessionId == 0)
            return;

        PlayerScoreStat& stat = Ensure(sessionId, playerType);
        if (combo > stat.mMaxCombo)
            stat.mMaxCombo = combo;
    }

    // 아군을 힐/버프한 시점을 기록해 이후 짧은 시간 내 처치에 어시스트
    void MarkSupport(uint32 sessionId, uint8 playerType, float now)
    {
        if (sessionId == 0)
            return;

        Ensure(sessionId, playerType).mLastSupportTime = now;
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
