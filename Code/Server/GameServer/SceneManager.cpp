#include "pch.h"
#include "SceneManager.h"
#include "Scene.h"
#include "ServerCore.h"
#include "GameMode.h"
#include "RoomManager.h"
void SceneManager::Initialize()
{
	//mActiveScene = make_shared<Scene>();
	//mActiveScene->Initialize();
	//mScenesBySession.clear();
	mLobbyScenesBySession.clear();
	mSceneBySession.clear();
	mActiveScene.reset();

	FactoryScene();
}

void SceneManager::FactoryScene()
{
	{	// LOBBYSCENE
		shared_ptr<Scene> lobbyScene = make_shared<LobbyScene>();
		lobbyScene->Initialize();
		
		mGameScenes[(size_t)SceneId::Lobby]=lobbyScene;
	}


	{	// GAMESCENE
		shared_ptr<Scene> firstScene = make_shared<FirstScene>();
		firstScene->Initialize();
		
		mGameScenes[(size_t)SceneId::FirstGame] = firstScene;
		mActiveScene = firstScene;
	}
	{

		shared_ptr<Scene> secondScene = make_shared<SecondScene>();
		secondScene->Initialize();
		
		mGameScenes[(size_t)SceneId::SecondGame] = secondScene;
	}



	{	// RESULTSCENE
		shared_ptr<Scene> victoryScene = make_shared<VictoryScene>();
		victoryScene->Initialize();
		
		mGameScenes[(size_t)SceneId::VGame] = victoryScene;
	}
	{
		shared_ptr<Scene> loseScene = make_shared<LoseScene>();
		loseScene->Initialize();
		
		mGameScenes[(size_t)SceneId::LGame] = loseScene;
	}

	

}

void SceneManager::TransitionToScene()
{
	if (mActiveScene)
	{
		shared_ptr<GameMode>& currentGameMode = mActiveScene->GetGameMode();
		if (currentGameMode && currentGameMode->IsSceneChanging()) {
			LoadScene(currentGameMode->GetTargetSceneId());
			currentGameMode->IsSceneChanging() = false;
		}
			
	}
}

void SceneManager::Update(float deltaTime)
{
	for (const auto& [sessionId, scene] : mLobbyScenesBySession)
	{
		if (scene == nullptr)
			continue;

		scene->Update(deltaTime);
	}

	if (mActiveScene)
	{
		mActiveScene->Update(deltaTime);
	}

	TransitionToScene();
}


void SceneManager::LoadScene(SceneId id)
{
	if(mActiveScene)
		mActiveScene->Release();

	mActiveScene = mGameScenes[(size_t)id];
	mActiveScene->mIsStarted = true;
}

void SceneManager::InitializeSession(uint64 sessionId)
{

	//if (mScenesBySession.find(sessionId) != mScenesBySession.end())
	if (mLobbyScenesBySession.find(sessionId) != mLobbyScenesBySession.end())
	{
		// 이미 초기화된 세션이라도 RoomManager 입장 후크는 중복 방어가 내부에 있으므로 호출
		if (mRoomManager) mRoomManager->OnSessionEnterLobby(sessionId);
		return;
	}

	auto scene = make_shared<Scene>();
	scene->Initialize();


	//mScenesBySession.emplace(sessionId, std::move(scene));
	mLobbyScenesBySession.emplace(sessionId, std::move(scene));
	mSceneBySession[sessionId] = SceneId::Lobby;

	// 세션이 로비에 들어오는 순간 RoomManager 에도 알린다
	if (mRoomManager) mRoomManager->OnSessionEnterLobby(sessionId);
}



void SceneManager::LoadScene(uint64 sessionId, wstring sceneName)
{
	// TODO : 기존 Scene 정리
	// TODO : 파일에서 Scene 정보 로드

	auto scene = make_shared<Scene>();
	scene->Initialize();
	//mScenesBySession[sessionId] = std::move(scene);
	mLobbyScenesBySession[sessionId] = std::move(scene);
	mSceneBySession[sessionId] = SceneId::Lobby;

}

shared_ptr<Scene> SceneManager::GetScene(uint64 sessionId) const
{

	//auto findIt = mScenesBySession.find(sessionId);
	//if (findIt == mScenesBySession.end())
	auto stateIt = mSceneBySession.find(sessionId);
	SceneId sceneState = SceneId::Lobby;
	if (stateIt != mSceneBySession.end())
	{
		sceneState = stateIt->second;
	}

	if (sceneState == SceneId::FirstGame)
	{
		return mActiveScene;
	}

	auto findIt = mLobbyScenesBySession.find(sessionId);
	if (findIt == mLobbyScenesBySession.end())
		return nullptr;

	return findIt->second;
}

SceneId SceneManager::GetOrCreateSceneState(uint64 sessionId)
{
	auto findIt = mSceneBySession.find(sessionId);
	if (findIt != mSceneBySession.end())
		return findIt->second;

	InitializeSession(sessionId);
	return SceneId::Lobby;
}

bool SceneManager::EnqueueCommand(const InputCommand& command)
{
	// Room 패킷은 World 가 아니라 RoomManager 가 직접 처리
	if (command.Type == PKT_Type::C2S_ROOM_READY ||
		command.Type == PKT_Type::C2S_ROOM_CHARACTER_SELECT)
	{
		return mRoomManager ? mRoomManager->HandleRoomPacket(command) : false;
	}

	if (command.Type == PKT_Type::C2S_SCENE_CHANGE)
		return HandleSceneChange(command);

	if (command.Type == PKT_Type::C2S_PKT_LOGIN)
	{
		GetOrCreateSceneState(command.SessionId);
	}

	auto scene = GetScene(command.SessionId);
	if (!scene)
	{
		InitializeSession(command.SessionId);
		scene = GetScene(command.SessionId);
	}

	if (!scene)
		return false;

	auto world = scene->GetWorld();
	if (!world)
		return false;

	return world->EnqueueCommand(command);
}

bool SceneManager::HandleSceneChange(const InputCommand& command)
{
	const C2S_SceneChangePacket* requestPacket = command.ViewAs<C2S_SceneChangePacket>();
	if (!requestPacket)
		return false;

	SceneId currentScene = GetOrCreateSceneState(command.SessionId);
	SceneId requestedScene = requestPacket->targetScene;

	// Host/Ready 자격 검사
	// (씬 전환 규칙 검사보다 먼저 — 거부 시 RoomErrorCode 와 함께 거절 응답)
	if (currentScene == SceneId::Lobby && requestedScene == SceneId::FirstGame && mRoomManager)
	{
		RoomErrorCode roomErr = RoomErrorCode::None;
		if (!mRoomManager->CanStartGame(command.SessionId, roomErr))
		{
			// 씬 전환 거부 응답
			S2C_SceneChangeResultPacket reject(currentScene, false);
			SendRequest rejReq{ static_cast<uint32>(command.SessionId), PKT_Type::S2C_SCENE_CHANGE_RESULT, sizeof(reject) };
			rejReq.StoreAs<S2C_SceneChangeResultPacket>(reject);
			gSendQueue.Push(rejReq);

			// Room 자격 오류 사유
			S2C_RoomErrorPacket errPkt;
			errPkt.roomId = 1; // v1: 단일 방
			errPkt.errorCode = static_cast<uint8>(roomErr);
			SendRequest errReq{ static_cast<uint32>(command.SessionId), PKT_Type::S2C_ROOM_ERROR, sizeof(errPkt) };
			errReq.StoreAs<S2C_RoomErrorPacket>(errPkt);
			gSendQueue.Push(errReq);
			return true;
		}
		// 자격 통과: ready 일괄 리셋 + RoomState 브로드캐스트 (세션은 방에 남겨둠)
		mRoomManager->OnGameStarted(command.SessionId);
	}

	const bool isApproved = IsSceneChangeAllowed(currentScene, requestedScene);
	if (isApproved)
	{
		mSceneBySession[command.SessionId] = requestedScene;
		if (requestedScene == SceneId::FirstGame && !mActiveScene)
		{
			auto scene = make_shared<Scene>();
			scene->Initialize();
			mActiveScene = std::move(scene);
		}
		else if (requestedScene == SceneId::Lobby)
		{
			if (mLobbyScenesBySession.find(command.SessionId) == mLobbyScenesBySession.end())
			{
				auto scene = make_shared<Scene>();
				scene->Initialize();
				mLobbyScenesBySession.emplace(command.SessionId, std::move(scene));
			}
		}
		currentScene = requestedScene;
	}

	S2C_SceneChangeResultPacket responsePacket(currentScene, isApproved);
	SendRequest response{ command.SessionId, PKT_Type::S2C_SCENE_CHANGE_RESULT, sizeof(S2C_SceneChangeResultPacket) };
	response.StoreAs<S2C_SceneChangeResultPacket>(responsePacket);
	gSendQueue.Push(response);

	// 위에서 요청자(Host)에게만 결과를 보냈으므로 나머지 방원에게도 동일한 승인 패킷 전송
	// mSceneBySession 매핑도 함께 갱신한다.
	if (isApproved && requestedScene == SceneId::FirstGame && mRoomManager)
	{
		RoomState* room = mRoomManager->GetRoomByPlayer(command.SessionId);
		if (room)
		{
			for (uint64 otherSessionId : room->GetSessionIds())
			{
				if (otherSessionId == command.SessionId) continue; // Host 는 위에서 처리 완료

				mSceneBySession[otherSessionId] = SceneId::FirstGame;

				S2C_SceneChangeResultPacket otherPkt(SceneId::FirstGame, true);
				SendRequest otherReq{ static_cast<uint32>(otherSessionId), PKT_Type::S2C_SCENE_CHANGE_RESULT, sizeof(otherPkt) };
				otherReq.StoreAs<S2C_SceneChangeResultPacket>(otherPkt);
				gSendQueue.Push(otherReq);
			}
		}
	}

	return true;
}

bool SceneManager::IsSceneChangeAllowed(SceneId currentScene, SceneId requestedScene) const
{
	if (currentScene == requestedScene)
		return false;

	switch (currentScene)
	{
	case SceneId::Lobby:
		return requestedScene == SceneId::FirstGame;
	case SceneId::FirstGame:
		return requestedScene == SceneId::Lobby;
	default:
		return false;
	}
}


void SceneManager::SetLayerName(uint8 index, const wstring& name)
{
	// 기존 데이터 삭제
	const wstring& prevName = _layerNames[index];
	_layerIndex.erase(prevName);

	_layerNames[index] = name;
	_layerIndex[name] = index;
}

uint8 SceneManager::LayerNameToIndex(const wstring& name)
{
	auto findIt = _layerIndex.find(name);
	if (findIt == _layerIndex.end())
		return 0;

	return findIt->second;
}


void SceneManager::RemoveSession(uint64 sessionId)
{
	// Scene 정리 전에 RoomManager 에서 빼야 호스트 승계가 정상 동작
	if (mRoomManager) mRoomManager->OnSessionLeave(sessionId);

	mLobbyScenesBySession.erase(sessionId);
	mSceneBySession.erase(sessionId);
}
