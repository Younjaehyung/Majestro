#pragma once
#include "System.h"

class NetRecvSystem : public System
{
public:
	NetRecvSystem(World* world);
	void Update(float dt) override;

private:
	void RecvInput(uint32 sessionId, const C2S_MovePacket& inputFrame);
	void RecvAction(uint32 sessionId, const C2S_ActionPacket& inputFrame);
	void RecvRhythmChanged(uint32 sessionId, const C2S_RhythmChangedPacket& inputFrame);

	void HandleGameStart(InputCommand& inputCommand);

	void SpawnPlayer(InputCommand& inputCommand);
	void SendSpawnToSelf(uint32 sessionId, Entity e, uint8 playerType);
	void BroadcastSpawnToOthers(uint32 sessionId, Entity e, uint8 playerType);
	void SendExistingPlayersToNewClient(uint32 newSessionId);

	void EnemySpawnProcess(InputCommand& inputCommand);
	void BulletPoolSpawnProcess(InputCommand& inputCommand);

	Entity FindEntityBySession(uint32 sessionId) const;
	std::vector<uint32> CollectPlayerSessions() const;

private:
	vector<uint64> mNetEntityIds{};
	vector<uint64> mBulletNetEntityIds{};
	bool mEnemySpawnOnce = true;
	bool mBulletSpawnOnce = true;

	InputCommand mInputCommand;
};
