#pragma once
#include "System.h"
#include "NetIdMap.h"
#include "PacketHelper.h"
#include <functional>
#include <array>

class  EventManager;

class NetRecvSystem : public System
{
public:
	NetRecvSystem(World* world) : System(world) { mPhase = SysPhase::Pre; }
	NetRecvSystem(World* world, shared_ptr<NetIdMap>& netIdMap);

	virtual ~NetRecvSystem();

	void Initialize();
	void Update(float deltaTime);

	void ProcessOne(const InputCommand& msg);

private:
	void RegisterHandlers();

	void HandleSpawn(const InputCommand& msg);
	void HandleDespawn(const InputCommand& msg);
	void HandleGimmickState(const InputCommand& msg);
	void HandleRhythmChanged(const InputCommand& msg);
	void HandleSync(const InputCommand& msg);
	void HandleBeatJudgement(const InputCommand& msg);
	void HandleComboChanged(const InputCommand& msg);
	void HandleMove(const InputCommand& msg);
	void HandleState(const InputCommand& msg);
	void HandleHealth(const InputCommand& msg);
	void HandleArmor(const InputCommand& msg);
	void HandleAmmo(const InputCommand& msg);
	void HandlePlayerStatus(const InputCommand& msg);
	void HandleCooldown(const InputCommand& msg);
	void HandleCollision(const InputCommand& msg);
	void HandleBulletActivate(const InputCommand& msg);
	void HandleBulletDeactivate(const InputCommand& msg);
	void HandleEffectSpawn(const InputCommand& msg);
	void HandleSticker(const InputCommand& msg);
	void HandleEmote(const InputCommand& msg);
	void HandleHitConfirm(const InputCommand& msg);
	void HandleGameStart(const InputCommand& msg);
	void HandleSceneChangeResult(const InputCommand& msg);
	void HandleSceneState(const InputCommand& msg);
	void HandlePrepareSceneState(const InputCommand& msg);
	void HandleConquestSceneState(const InputCommand& msg);
	void HandleEscortSceneState(const InputCommand& msg);
	void HandleClearSceneState(const InputCommand& msg);
	void HandleScoreBoard(const InputCommand& msg);
	void HandleReplicationDelta(const InputCommand& msg);
	void HandleRoomState(const InputCommand& msg);
	void HandleRoomError(const InputCommand& msg);
	void HandleRoomList(const InputCommand& msg);      
	void HandleRoomJoinResult(const InputCommand& msg);

	Entity CreateEntityFromArchetype(uint32_t archetypeId, const InputCommand& spawnCommand);

private:
	using Handler = std::function<void(const InputCommand&)>;
	std::array<Handler, static_cast<size_t>(KMSG) + 1> mHandlers{};

	shared_ptr<NetIdMap> mNetIdMap = nullptr;

	InputCommand mInputCommand{};

	bool mIsPlayer = false;
	SceneId mCurrentScene = SceneId::Lobby;

	bool mStopProcessing = false;

};

