#include "pch.h"
#include "ScoreSystem.h"

#include "World.h"
#include "EventManager.h"
#include "GameEvents.h"
#include "ScoreBoardComponent.h"
#include "GameRuleComponent.h"
#include "NetEntityComponent.h"
#include "PlayerComponent.h"
#include "GameTimer.h"

#include <unordered_set>

ScoreSystem::ScoreSystem(World* world) : System(world)
{
    if (mWorld && mWorld->GetSingleton<ScoreBoardComponent>() == nullptr)
        mWorld->AddSingleton<ScoreBoardComponent>();
}

void ScoreSystem::Update(float /*deltaTime*/)
{
    const float now = GetServerTotalTimeSeconds();


    SampleMaxCombo();


    ConsumeSupportActions(now);
    ConsumeEnemyKills(now);
}

void ScoreSystem::SampleMaxCombo()
{
    if (!mWorld->HasComponentPool<NetEntityComponent>() ||
        !mWorld->HasComponentPool<MainPlayerComponent>())
        return;

    ScoreBoardComponent* scoreBoard = mWorld->GetSingleton<ScoreBoardComponent>();
    if (!scoreBoard)
        return;

    for (Entity entity : mWorld->GetEntitiesWithComponents<NetEntityComponent, MainPlayerComponent>())
    {
        const NetEntityComponent* netComp = mWorld->GetComponent<NetEntityComponent>(entity);
        const MainPlayerComponent* playerComp = mWorld->GetComponent<MainPlayerComponent>(entity);
        if (!netComp || !playerComp || netComp->mSessionId == 0)
            continue;

        scoreBoard->UpdateMaxCombo(
            netComp->mSessionId,
            static_cast<uint8>(playerComp->mPlayerType),
            playerComp->mRhythmCombo);
    }
}

void ScoreSystem::ConsumeSupportActions(float now)
{
    EventManager* em = mWorld->GetEventManager().get();
    ScoreBoardComponent* scoreBoard = mWorld->GetSingleton<ScoreBoardComponent>();
    if (!em || !scoreBoard)
        return;

    em->Consume<EvSupportAction>([&](const EvSupportAction& e)
    {
        if (!e.player.IsValid())
            return;

        const NetEntityComponent* netComp = mWorld->GetComponent<NetEntityComponent>(e.player);
        const MainPlayerComponent* playerComp = mWorld->GetComponent<MainPlayerComponent>(e.player);
        if (!netComp || !playerComp || netComp->mSessionId == 0)
            return;

        scoreBoard->MarkSupport(
            netComp->mSessionId,
            static_cast<uint8>(playerComp->mPlayerType),
            now);
    });
}

void ScoreSystem::ConsumeEnemyKills(float now)
{
    EventManager* em = mWorld->GetEventManager().get();
    ScoreBoardComponent* scoreBoard = mWorld->GetSingleton<ScoreBoardComponent>();
    if (!em || !scoreBoard)
        return;

    em->Consume<EvEnemyKilled>([&](const EvEnemyKilled& e)
    {
        // 막타 처치
        if (e.killerSession != 0)
        {
            const int32 awardedScore =
                scoreBoard->RecordKill(e.killerSession, e.killerType, e.enemyType);

            if (GameRuleComponent* rule = mWorld->GetSingleton<GameRuleComponent>())
                rule->mPlayerScore += awardedScore;
        }

        // 어시스트
        std::unordered_set<uint32> assistSessions;
        for (uint8 i = 0; i < e.contributorCount; ++i)
        {
            const uint32 session = e.contributors[i];
            if (session != 0 && session != e.killerSession)
                assistSessions.insert(session);
        }
        for (const auto& [session, stat] : scoreBoard->mPlayerStats)
        {
            if (session == e.killerSession)
                continue;
            if (now - stat.mLastSupportTime <= ScoreBoardComponent::kSupportAssistWindow)
                assistSessions.insert(session);
        }
        for (uint32 session : assistSessions)
            scoreBoard->AddAssist(session);
    });
}
