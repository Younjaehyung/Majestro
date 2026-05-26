#include "pch.h"
#include "GameMode.h"
#include "Scene.h"
#include "Engine.h"
#include "Network.h"
#include "InputManager.h"
#include "GameRuleComponent.h"
#include "LobbyRoomStateComponent.h"

void LobbyGameMode::Initialize()
{

}

void LobbyGameMode::PreUpdate(float deltaTime)
{
	if (!INPUT.GetKeyDown(eKeyCode::G)) return;

	auto world = mScene->GetWorld();

	// 본인이 Host 가 아니거나 전원 Ready 가 아니면 enqueue 중단
	// 서버에서도 동일 검사를 수행하긴 함
	if (world->HasComponentPool<LobbyRoomStateComponent>())
	{
		auto entities = world->GetEntitiesWithComponent<LobbyRoomStateComponent>();
		if (!entities.empty())
		{
			LobbyRoomStateComponent* state = world->GetComponent<LobbyRoomStateComponent>(entities[0]);
			if (state && state->mHasSnapshot)
			{
				const uint32 myClientId = Network::GetInstance().mClientId;
				bool isMeHost = false;
				bool allReady = (state->mPlayerCount > 0);
				for (uint8 i = 0; i < state->mPlayerCount && i < ROOM_MAX_PLAYERS; ++i)
				{
					if (state->mSlots[i].sessionId == myClientId && state->mSlots[i].isHost)
						isMeHost = true;
					if (!state->mSlots[i].ready)
						allReady = false;
				}
				if (!isMeHost || !allReady)
				{
					return;
				}
			}
		}
	}

	world->GetEventManager()->Enqueue(EvNetSceneChange{ SceneId::FirstGame });
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
