#include "pch.h"
#include "GameMode.h"
#include "Scene.h"
#include "Engine.h"
#include "Network.h"
#include "InputManager.h"
#include "GameRuleComponent.h"
#include "LobbyRoomStateComponent.h"
#include "LobbyRoomListComponent.h"

void LobbyGameMode::Initialize()
{

}

void LobbyGameMode::PreUpdate(float deltaTime)
{
	if (!INPUT.GetKeyDown(eKeyCode::G)) return;

	auto world = mScene->GetWorld();

	// 방 미입장 상태에서는 게임 시작 불가
	if (world->HasComponentPool<LobbyRoomListComponent>())
	{
		auto listEntities = world->GetEntitiesWithComponent<LobbyRoomListComponent>();
		if (!listEntities.empty())
		{
			auto* listComp = world->GetComponent<LobbyRoomListComponent>(listEntities[0]);
			if (listComp && listComp->mCurrentRoomId == 0)
				return;
		}
	}

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

	world->GetEventManager()->Enqueue(EvNetSceneChange{ SceneId::Plaza });
}


void PlazaGameMode::Initialize()
{

}

void PlazaGameMode::PreUpdate(float deltaTime)
{
	// 레벨 진입 요청 (임시)
	if (INPUT.GetKeyDown(eKeyCode::F5))
		mScene->GetWorld()->GetEventManager()->Enqueue(EvNetSceneChange{ SceneId::FirstGame });
	if (INPUT.GetKeyDown(eKeyCode::F6))
		mScene->GetWorld()->GetEventManager()->Enqueue(EvNetSceneChange{ SceneId::SecondGame });
	if (INPUT.GetKeyDown(eKeyCode::F7))
		mScene->GetWorld()->GetEventManager()->Enqueue(EvNetSceneChange{ SceneId::ThirdGame });
	if (INPUT.GetKeyDown(eKeyCode::F8))
		mScene->GetWorld()->GetEventManager()->Enqueue(EvNetSceneChange{ SceneId::FourthGame });
}




void MenuGameMode::Initialize()
{
	mTargetSceneId = SceneId::Lobby;
}

void MenuGameMode::PreUpdate(float deltaTime)
{
}

void WaveGameMode::Initialize()
{

}

void WaveGameMode::PreUpdate(float deltaTime)
{	// 씬전환 디버그
	// [디버그]
	if (mScene->GetSceneId() == SceneId::FirstGame && INPUT.GetKeyDown(eKeyCode::F5))
	{
		mScene->GetWorld()->GetEventManager()->Enqueue(EvNetSceneChange{ SceneId::SecondGame });
	}
	// [디버그]
	if (mScene->GetSceneId() == SceneId::SecondGame && INPUT.GetKeyDown(eKeyCode::F5))
	{
		mScene->GetWorld()->GetEventManager()->Enqueue(EvNetSceneChange{ SceneId::ThirdGame });
	}
	// [디버그]
	if (mScene->GetSceneId() == SceneId::ThirdGame && INPUT.GetKeyDown(eKeyCode::F5))
	{
		mScene->GetWorld()->GetEventManager()->Enqueue(EvNetSceneChange{ SceneId::FourthGame });
	}
}

void WaveGameMode::PostUpdate(float deltaTime)
{

	if (mScene == nullptr) return;

	
}


void ResultGameMode::Initialize()
{

}
