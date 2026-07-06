#pragma once
#include "AIManager.h"
#include "SceneManager.h"
#include "ResourceManager.h"
#include "RoomManager.h"
#include "RoomNotifier.h"
#include "RoomPacketHandler.h"
#include "ScenePacketHandler.h"
#include "PacketRouter.h"

class GameCore
{
public:
	void Initialize();
	void Start();
	void Update(float deltaTime);
	SceneManager& GetSceneManager() { return *mSceneManager; };
	ResourceManager& GetResourceManager() { return *mResourceManager; };
	AIManager& GetAIManager() { return *mAIManager; };
	RoomManager& GetRoomManager() { return *mRoomManager; };

private:
	void LoadGameData();
	void UpdateGameLogic(float deltaTime);

private:

	std::unique_ptr<AIManager> mAIManager;
	std::unique_ptr<SceneManager> mSceneManager;
	std::unique_ptr<ResourceManager> mResourceManager;
	std::unique_ptr<RoomManager> mRoomManager;

	// 네트워크-로직 경계: 패킷 파싱/송신 어댑터 계층
	std::unique_ptr<RoomNotifier> mRoomNotifier;
	std::unique_ptr<RoomPacketHandler> mRoomPacketHandler;
	std::unique_ptr<ScenePacketHandler> mScenePacketHandler;
	std::unique_ptr<PacketRouter> mPacketRouter;
};

