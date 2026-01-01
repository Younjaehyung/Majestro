#include "pch.h"
#include "GameCore.h"

void GameCore::Initialize()
{
	mSceneManager = std::make_unique<SceneManager>();
}

void GameCore::Start()
{
	mSceneManager->Initialize();
	LoadGameData();
}

void GameCore::Update(float deltaTime)
{
	UpdateGameLogic(deltaTime);
	mSceneManager->Update(deltaTime);
}

void GameCore::LoadGameData()
{
}

void GameCore::UpdateGameLogic(float deltaTime)
{
}
