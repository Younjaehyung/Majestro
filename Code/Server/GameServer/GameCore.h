#pragma once
#include "AIManager.h"
#include "SceneManager.h"
#include "ResourceManager.h"
#include "RoomManager.h"

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
};

