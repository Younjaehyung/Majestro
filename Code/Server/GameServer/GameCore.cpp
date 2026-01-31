#include "pch.h"
#include "GameCore.h"
#include "ServerCore.h"

void GameCore::Initialize()
{
	mResourceManager = std::make_unique<ResourceManager>();
	mSceneManager = std::make_unique<SceneManager>();
	
}

void GameCore::Start()
{
	cout << "load gamedata" << endl;
	LoadGameData();
	cout << "load gamedata2" << endl;
}

void GameCore::Update(float deltaTime)
{
	UpdateGameLogic(deltaTime);
	uint32 sessionId = 0;
	while (gNewSessions.Pop(sessionId))
	{
		mSceneManager->InitializeSession(sessionId);
	}
	mSceneManager->Update(deltaTime);
}

void GameCore::LoadGameData()
{
	mResourceManager->Initialize();
	mSceneManager->Initialize();
}

void GameCore::UpdateGameLogic(float deltaTime)
{
}
