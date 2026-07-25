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

	// 세션 이탈/접속 종료 시 호출
	// 해당 세션의 플레이어 엔티티를 같은 World 의 다른 플레이어들에게 despawn 통지 후 파괴.
	void DespawnPlayerBySession(uint32 sessionId);

private:
	void SendMove(float dt);
	void SendAction();
	void SendPlayerStatus();
	void SendCollision();
	void SendHealthEvents();
	void SendArmorEvents();
	void SendAmmoEvents();
	void SendCooldownEvents();
	void SendBulletDeactivateEvents();
	void SendEffectSpawnEvents();
	void SendBossTileEvents();
	void SendStickerEvents();
	void SendEmoteEvents();
	void SendChatEvents();
	void SendHitConfirmEvents();
	void SendGimmickStateEvents();
	void SendRhythmChangedEvents();

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

	template<typename T>
	void Broadcast(const std::vector<uint32>& recipients, PKT_Type type, const T& payload)
	{
		for (uint32 sessionId : recipients)
		{
			mSendReq.SessionId = sessionId;
			mSendReq.Type = type;
			mSendReq.StoreAs<T>(payload);   // StoreAs 내부에서 Size 도 sizeof(T) 로 설정됨
			gSendQueue.Push(mSendReq);
		}
	}


	template<typename T>
	void Broadcast(PKT_Type type, const T& payload)
	{
		Broadcast(CollectPlayerSessions(), type, payload);
	}
private:
	SendRequest mSendReq;
	std::unordered_set<uint32> mSessionSet;

private:
	static constexpr float mMoveSendHz = 30.0f;
	static constexpr int mMaxMoveBurst = 4;
	float mMoveSendAccumulator = 0.0f;
	float mMoveSendInterval = 1.0f / mMoveSendHz;

	uint32 mSequence = 0;

	double mServerClockSec = 0.0;	// 실시간 누적 서버 클럭(초)

	RateLimiter mMovementRate{ 30.f };  // 이동 입력 30Hz
	RateLimiter mPlayerStatusRate{ 10.f };

};

