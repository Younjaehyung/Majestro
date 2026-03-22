#include "pch.h"
#include "GameMode.h"
#include "Scene.h"
#include "Engine.h"
#include "Network.h"
#include "InputManager.h"

void LobbyGameMode::Initialize()
{
	
}

void LobbyGameMode::PreUpdate(float deltaTime)
{
	if (INPUT.GetKeyDown(eKeyCode::G)) {
		mScene->GetWorld()->GetEventManager()->Enqueue(EvNetSceneChange{ SceneId::FirstGame });
	}
}


void WaveGameMode::Initialize()
{

}


void ResultGameMode::Initialize()
{
	
}


void MenuGameMode::Initialize()
{
	mTargetSceneId = SceneId::Lobby;
}

void MenuGameMode::PreUpdate(float deltaTime)
{
	if (INPUT.GetKeyDown(eKeyCode::SPACE)) {
		Network::GetInstance().Awake();
		mTargetSceneId = SceneId::Lobby;
		IsSceneChanging() = true;
	}
}
