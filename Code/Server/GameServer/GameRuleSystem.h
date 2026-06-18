#pragma once
#include "System.h"
#include "GameMode.h"

class GamePreRuleSystem : public System
{
public:
	GamePreRuleSystem(World* world, shared_ptr<GameMode> gameMode);
	void Update(float deltaTime) override;

private:
	shared_ptr<GameMode> mGameMode;

};



class GamePostRuleSystem : public System
{
public:
	GamePostRuleSystem(World* world, shared_ptr<GameMode> gameMode);
	void Update(float deltaTime) override;

private:
	shared_ptr<GameMode> mGameMode;
};



class GameNetRuleSystem : public System
{
public:
	GameNetRuleSystem(World* world, shared_ptr<GameMode> gameMode);
	void Update(float deltaTime) override;

private:

	void SendSceneState(Entity rule);
	void SendScenePrepare(Entity rule);
	void SendSceneConquest(Entity rule);
	void SendSceneEscort(Entity rule);
	void SendSceneClear(Entity rule);

private:
	template<typename PktT>
	void Broadcast(PKT_Type type, const PktT& pkt)
	{
		for (uint32 sid : mSessionSet)
		{
			SendRequest req{};
			req.SessionId = sid;
			req.Type = type;
			req.Size = sizeof(PktT);
			req.StoreAs<PktT>(pkt);
			gSendQueue.Push(req);
		}
	}

	void CollectPlayerSessions();
private:
	std::unordered_set<uint32> mSessionSet;
	shared_ptr<GameMode> mGameMode;

	RateLimiter mSceneStateSendRate{ 60.f };
	RateLimiter mScenePhaseSendRate{ 60.f };
};
