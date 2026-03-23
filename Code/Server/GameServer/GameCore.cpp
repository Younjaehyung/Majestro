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

	InputCommand command;
	while (gRecvQueue.Pop(command))
	{
		mSceneManager->EnqueueCommand(command);
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
