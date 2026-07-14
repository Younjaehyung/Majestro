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
	void RecvEmote(uint32 sessionId, const C2S_EmotePacket& packet);
	void RecvSync(uint32 sessionId, const C2S_SyncPacket& inputFrame);

	void HandleGameStart(InputCommand& inputCommand);

	Entity SpawnPlayer(InputCommand& inputCommand);
	void EnsureBulletPool(InputCommand& inputCommand);

	Entity FindEntityBySession(uint32 sessionId) const;


	bool IsNewerSeq(uint32 lhs, uint32 rhs){return static_cast<int32>(lhs - rhs) > 0;}
private:
	bool mBulletSpawnOnce = true;

	InputCommand mInputCommand;
};
