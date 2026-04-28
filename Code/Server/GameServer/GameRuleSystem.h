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

	//template<typename T>
	//void Broadcast(PKT_Type type, const T& packet)
	//{
	//	for (uint32 sessionId : mRecipients)
	//	{
	//		SendRequest req{};
	//		req.SessionId = sessionId;
	//		req.Type = type;
	//		req.Size = sizeof(T);
	//		req.StoreAs<T>(packet);

	//		gSendQueue.Push(req);
	//	}
	//}

private: 
	void SendSceneState(float deltaTime);

	void CollectPlayerSessions();
private:
	std::unordered_set<uint32> mSessionSet;
	shared_ptr<GameMode> mGameMode;
};