#include "pch.h"
#include "GameMode.h"
#include "Scene.h"
#include "Engine.h"
#include "Network.h"
#include "InputManager.h"
#include "GameRuleComponent.h"

void LobbyGameMode::Initialize()
{
	
}

void LobbyGameMode::PreUpdate(float deltaTime)
{
	if (INPUT.GetKeyDown(eKeyCode::G)) {
		mScene->GetWorld()->GetEventManager()->Enqueue(EvNetSceneChange{ SceneId::FirstGame });
	}
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

void WaveGameMode::Initialize()
{

}

void WaveGameMode::PreUpdate(float deltaTime)
{
	Entity e = mScene->GetWorld()->GetGameRuleEntity(); // 게임 규칙 엔티티에서 필요한 정보 가져오기



}

void WaveGameMode::PostUpdate(float deltaTime)
{

	if (mScene == nullptr) return;

	
}


void ResultGameMode::Initialize()
{

}
