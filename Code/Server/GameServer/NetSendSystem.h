#pragma once
#include "System.h"
#include "ServerCore.h"

class NetEntityComponent;
struct SendRequest;




class NetSendSystem : public System
{
public:
	NetSendSystem(World* world);
	void Update(float dt) override;

private:
	void SendMove(NetEntityComponent*, SendRequest* , float);
	void SendAction();
	void SendCollision();
	void SendHealthEvents();
	void SendArmorEvents();
	void SendAmmoEvents();
	void SendBulletDeactivateEvents();
	void SendEffectSpawnEvents();
	void SendHitConfirmEvents();

	// 신규 세션 입장 시 EvSessionJoined 를 소비해서 초기 World 상태를 송신.
	void HandleSessionJoinedEvents();
	void SendPlayerSelfSpawn(uint32 sessionId, Entity playerEntity, uint8 playerType);
	void BroadcastPlayerToOthers(uint32 sessionId, Entity playerEntity, uint8 playerType);
	void SendExistingPlayersToNewSession(uint32 newSessionId);
	void SendWorldObjectsToNewSession(uint32 newSessionId);
	void SendEnemyPoolToNewSession(uint32 newSessionId);
	void SendBulletPoolToNewSession(uint32 newSessionId);
	void SendHealthSnapshotToNewSession(uint32 newSessionId);
	void SendArmorSnapshotToNewSession(uint32 newSessionId);

	std::vector<uint32> CollectPlayerSessions();
private:
	SendRequest mSendReq;
	NetEntityComponent* mNetComp = nullptr;
	std::unordered_set<uint32> mSessionSet;

private:
	static constexpr float mMoveSendHz = 30.0f;
	static constexpr int mMaxMoveBurst = 4;
	float mMoveSendAccumulator = 0.0f;
	float mMoveSendInterval = 1.0f / mMoveSendHz;

	uint32 mSequence = 0;

	RateLimiter mMovementRate{ 30.f };  // 이동 입력 30Hz

};

