#include "pch.h"
#include "GameCore.h"

void GameCore::Initialize()
{
	mResourceManager = std::make_unique<ResourceManager>();
	mSceneManager = std::make_unique<SceneManager>();
	
}

void GameCore::Start()
{
	
	LoadGameData();

}

void GameCore::Update(float deltaTime)
{
	UpdateGameLogic(deltaTime);
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
