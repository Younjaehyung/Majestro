#include "pch.h"
#include "SceneManager.h"
#include "Scene.h"
#include "ServerCore.h"
#include "GameMode.h"
#include "RoomManager.h"
#include "World.h"
#include "NetSendSystem.h"

void SceneManager::Initialize()
{

	mLobbyScenesBySession.clear();
	mSceneBySession.clear();
	mGameWorldsByRoom.clear();

	FactoryScene();
}

void SceneManager::FactoryScene()
{
	{	// LOBBYSCENE
		shared_ptr<Scene> lobbyScene = make_shared<LobbyScene>();
		lobbyScene->Initialize();
		
		mGameScenes[(size_t)SceneId::Lobby]=lobbyScene;
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


shared_ptr<Scene> SceneManager::CreateSceneById(SceneId id)
{
	switch (id)
	{
	case SceneId::Plaza:      return make_shared<PlazaScene>();
	case SceneId::FirstGame:  return make_shared<FirstScene>();
	case SceneId::SecondGame: return make_shared<SecondScene>();
	case SceneId::ThirdGame:  return make_shared<ThirdScene>();
	default:                  return nullptr;
	}
}

void SceneManager::TransitionToScene()
{
	// 게임이 끝난(GameMode 가 씬 전환 요청한) 방들을 수집 (목적지/출발 씬, 실패 여부 포함)
	struct FinishedEntry { uint32 roomId; SceneId target; SceneId from; bool failed; };
	std::vector<FinishedEntry> finished;
	for (auto& [roomId, scene] : mGameWorldsByRoom)
	{
		if (scene == nullptr) continue;
		shared_ptr<GameMode>& gm = scene->GetGameMode();
		if (gm && gm->IsSceneChanging())
		{
			gm->IsSceneChanging() = false;
			finished.push_back({ roomId, gm->GetTargetSceneId(), scene->GetSceneId(), gm->IsFailed() });
		}
	}

	// 광장/다음 레벨로 가는 방은 World 만 교체(방 유지), 그 외는 게임 종료 + 방 해체
	std::vector<std::pair<uint32, SceneId>> closing;
	for (auto& f : finished)
	{
		if (IsRoomScene(f.target))
		{
			// 레벨 클리어로 광장 복귀 시 다음 스테이지 해금 (실패 복귀는 진행도 유지)
			if (f.target == SceneId::Plaza && IsLevelScene(f.from) && !f.failed && mRoomManager)
			{
				if (RoomState* room = mRoomManager->GetRoom(f.roomId))
				{
					const uint8 clearedIdx = StageIndexOf(f.from);
					if (clearedIdx != INVALID_STAGE_INDEX && clearedIdx + 1 > room->mNextStageIndex)
						room->mNextStageIndex = clearedIdx + 1;
				}
			}
			SwapRoomWorldTo(f.roomId, f.target);
		}
		else
			closing.push_back({ f.roomId, f.target });
	}

	// 게임 종료
	for (auto& [roomId, target] : closing)
	{
		RoomState* room = mRoomManager ? mRoomManager->GetRoom(roomId) : nullptr;

		std::vector<uint64> sessions = room ? room->GetSessionIds() : std::vector<uint64>{};

		// 각 방원에게 목적지 씬 통지 + 서버측 씬 매핑 정리
		for (uint64 s : sessions)
		{
			S2C_SceneChangeResultPacket resultPkt(target, true);
			SendRequest req{ static_cast<uint32>(s), PKT_Type::S2C_SCENE_CHANGE_RESULT, sizeof(resultPkt) };
			req.StoreAs<S2C_SceneChangeResultPacket>(resultPkt);
			gSendQueue.Push(req);

			mSceneBySession.erase(s);
			mLobbyScenesBySession.erase(s);
		}

		// 방 전용 게임 World Release
		auto gIt = mGameWorldsByRoom.find(roomId);
		if (gIt != mGameWorldsByRoom.end())
		{
			if (gIt->second) gIt->second->Release();
			mGameWorldsByRoom.erase(gIt);
		}

		// 방 해체: 전원을 방에서 제거 + 빈 방 자동 소멸 + 홀 목록 갱신
		if (mRoomManager)
			for (uint64 s : sessions)
				mRoomManager->OnSessionLeave(s);
	}
}

void SceneManager::SwapRoomWorldTo(uint32 roomId, SceneId target)
{
	
	auto nextScene = CreateSceneById(target);
	if (!nextScene)	// 게임 씬이 아니면 World 교체 대상이 아님
		return;

	RoomState* room = mRoomManager ? mRoomManager->GetRoom(roomId) : nullptr;
	std::vector<uint64> sessions = room ? room->GetSessionIds() : std::vector<uint64>{};

	// 새 target 게임 씬 World 생성
	nextScene->Initialize();
	nextScene->mIsStarted = true;

	// 기존 게임 World 해제 후 같은 roomId 에 교체
	auto gIt = mGameWorldsByRoom.find(roomId);
	if (gIt != mGameWorldsByRoom.end() && gIt->second)
		gIt->second->Release();
	mGameWorldsByRoom[roomId] = nextScene;

	// 전원 씬 상태 갱신 + target 전환 통지 (클라가 로딩 후 재스폰)
	for (uint64 s : sessions)
	{
		mSceneBySession[s] = target;

		S2C_SceneChangeResultPacket pkt(target, true);
		SendRequest req{ static_cast<uint32>(s), PKT_Type::S2C_SCENE_CHANGE_RESULT, sizeof(pkt) };
		req.StoreAs<S2C_SceneChangeResultPacket>(pkt);
		gSendQueue.Push(req);
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

	// 모든 룸의 게임 World 를 각각 갱신
	for (auto& [roomId, scene] : mGameWorldsByRoom)
	{
		if (scene == nullptr)
			continue;

		scene->Update(deltaTime);
	}

	TransitionToScene();
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
	auto stateIt = mSceneBySession.find(sessionId);
	SceneId sceneState = SceneId::Lobby;
	if (stateIt != mSceneBySession.end())
	{
		sceneState = stateIt->second;
	}

	if (IsRoomScene(sceneState))
	{
		uint32 roomId = mRoomManager ? mRoomManager->GetRoomIdByPlayer(sessionId) : 0;
		auto gIt = mGameWorldsByRoom.find(roomId);
		return gIt != mGameWorldsByRoom.end() ? gIt->second : nullptr;
	}

	// 로비 / 홀
	auto findIt = mLobbyScenesBySession.find(sessionId);
	if (findIt == mLobbyScenesBySession.end())
		return nullptr;

	return findIt->second;
}

shared_ptr<Scene> SceneManager::GetGameWorld(uint32 roomId) const
{
	auto gIt = mGameWorldsByRoom.find(roomId);
	return gIt != mGameWorldsByRoom.end() ? gIt->second : nullptr;
}

SceneId SceneManager::GetOrCreateSceneState(uint64 sessionId)
{
	auto findIt = mSceneBySession.find(sessionId);
	if (findIt != mSceneBySession.end())
		return findIt->second;

	InitializeSession(sessionId);
	return SceneId::Lobby;
}

bool SceneManager::EnqueueToWorld(const InputCommand& command)
{
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

SceneChangeOutcome SceneManager::TryChangeScene(uint64 sessionId, SceneId requestedScene)
{
	SceneChangeOutcome outcome;
	SceneId currentScene = GetOrCreateSceneState(sessionId);

	// Host/Ready 자격 검사 — 로비에서 광장으로 출발할 때
	// (씬 전환 규칙 검사보다 먼저 — 거부 시 RoomErrorCode 와 함께 거절 응답)
	if (currentScene == SceneId::Lobby && requestedScene == SceneId::Plaza && mRoomManager)
	{
		RoomErrorCode roomErr = RoomErrorCode::None;
		if (!mRoomManager->CanStartGame(sessionId, roomErr))
		{
			outcome.sendResponse = true;
			outcome.approved = false;
			outcome.resultScene = currentScene;
			outcome.roomError = roomErr;
			outcome.errorRoomId = mRoomManager->GetRoomIdByPlayer(sessionId);
			return outcome;
		}
		// 자격 통과: ready 일괄 리셋 + RoomState 브로드캐스트 (세션은 방에 남겨둠)
		mRoomManager->OnGameStarted(sessionId);
	}

	// 광장 → 레벨 진입: Host 요청 + 해금 순서 검증 후 방 World 전환 예약.
	// 전환·통지는 다음 Update 의 TransitionToScene → SwapRoomWorldTo 가 전원에게 일괄 수행한다.
	if (currentScene == SceneId::Plaza && IsLevelScene(requestedScene) && mRoomManager)
	{
		RoomState* room = mRoomManager->GetRoomByPlayer(sessionId);
		// 관문지기 레벨 선택 UI 로 스테이지 자유 선택 — Host 요청이면 어느 레벨이든 허용.
		const bool allowed = room && room->IsHost(sessionId);
		if (!allowed)
		{
			outcome.sendResponse = true;
			outcome.approved = false;
			outcome.resultScene = currentScene;
			return outcome;
		}

		if (auto scene = GetGameWorld(room->mRoomId))
			if (auto& gameMode = scene->GetGameMode())
				gameMode->RequestTransition(requestedScene);
		return outcome;	// 응답 없음 — SwapRoomWorldTo 브로드캐스트가 결과 통지
	}

	// [디버그] 레벨 씬 강제 전환 요청 — 응답 패킷 없음
	if (IsLevelScene(currentScene) && IsLevelScene(requestedScene) && currentScene != requestedScene)
	{
		uint32 roomId = mRoomManager ? mRoomManager->GetRoomIdByPlayer(sessionId) : 0;
		auto gIt = mGameWorldsByRoom.find(roomId);
		if (gIt != mGameWorldsByRoom.end() && gIt->second)
		{
			if (auto& gameMode = gIt->second->GetGameMode())
			{
				gameMode->DebugForceTransition(requestedScene);
				cout << "[DEBUG] 강제 씬 전환: room=" << roomId
					<< " scene=" << (int)currentScene << " -> " << (int)requestedScene << endl;
			}
		}
		return outcome;
	}

	const bool isApproved = IsSceneChangeAllowed(currentScene, requestedScene);
	if (isApproved)
	{
		mSceneBySession[sessionId] = requestedScene;
		if (requestedScene == SceneId::Plaza)	// 로비 → 광장: 방 전용 World 생성
		{
			uint32 roomId = mRoomManager ? mRoomManager->GetRoomIdByPlayer(sessionId) : 0;
			if (mGameWorldsByRoom.find(roomId) == mGameWorldsByRoom.end())
			{
				auto scene = CreateSceneById(requestedScene);
				if (scene)
				{
					scene->Initialize();
					scene->mIsStarted = true;
					mGameWorldsByRoom[roomId] = std::move(scene);
				}
			}
		}
		else if (requestedScene == SceneId::Lobby)
		{
			if (mLobbyScenesBySession.find(sessionId) == mLobbyScenesBySession.end())
			{
				auto scene = make_shared<Scene>();
				scene->Initialize();
				mLobbyScenesBySession.emplace(sessionId, std::move(scene));
			}
		}
		currentScene = requestedScene;
	}

	outcome.sendResponse = true;
	outcome.approved = isApproved;
	outcome.resultScene = currentScene;

	// 요청자(Host) 외 나머지 방원도 함께 광장으로 — mSceneBySession 갱신, 통지는 핸들러가 수행
	if (isApproved && requestedScene == SceneId::Plaza && mRoomManager)
	{
		RoomState* room = mRoomManager->GetRoomByPlayer(sessionId);
		if (room)
		{
			for (uint64 otherSessionId : room->GetSessionIds())
			{
				if (otherSessionId == sessionId) continue; // Host 는 응답으로 처리

				mSceneBySession[otherSessionId] = SceneId::Plaza;
				outcome.alsoNotifySessions.push_back(otherSessionId);
			}
		}
	}

	return outcome;
}

bool SceneManager::IsSceneChangeAllowed(SceneId currentScene, SceneId requestedScene) const
{
	if (currentScene == requestedScene)
		return false;

	if (currentScene == SceneId::Lobby)	// 로비 -> 광장 출발
		return requestedScene == SceneId::Plaza;
	if (currentScene == SceneId::Plaza)	// 광장 -> 로비 이탈 (레벨 진입은 TryChangeScene 에서 별도 처리)
		return requestedScene == SceneId::Lobby;
	if (IsLevelScene(currentScene))		// 레벨 -> 로비 포기
		return requestedScene == SceneId::Lobby;
	return false;
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
	uint32 cachedRoomId = mRoomManager ? mRoomManager->GetRoomIdByPlayer(sessionId) : 0;

	
	if (cachedRoomId != 0)
	{
		if (auto gameScene = GetGameWorld(cachedRoomId))
		{
			// 방 게임 World 에서 플레이어 엔티티를 다른 클라에 despawn 통지 후 파괴
			if (auto world = gameScene->GetWorld())
			{
				if (auto* netSend = world->GetSystemManager()->GetSystem<NetSendSystem>())
					netSend->DespawnPlayerBySession(static_cast<uint32>(sessionId));
			}
		}
	}

	if (mRoomManager) mRoomManager->OnSessionLeave(sessionId);

	mLobbyScenesBySession.erase(sessionId);
	mSceneBySession.erase(sessionId);

	// 방의 마지막 씬이 나갔으면 게임 World Release
	// 게임 결과로 나중에 바꿔야 할듯
	if (cachedRoomId != 0)
		CleanupRoomWorldIfEmpty(cachedRoomId);
}

void SceneManager::CleanupRoomWorldIfEmpty(uint32 roomId)
{
	RoomState* room = mRoomManager ? mRoomManager->GetRoom(roomId) : nullptr;

	int inGame = 0;
	if (room)
	{
		for (uint64 s : room->GetSessionIds())
		{
			auto it = mSceneBySession.find(s);
			if (it != mSceneBySession.end() && IsRoomScene(it->second))
				++inGame;
		}
	}

	if (inGame == 0)
	{
		auto gIt = mGameWorldsByRoom.find(roomId);
		if (gIt != mGameWorldsByRoom.end())
		{
			if (gIt->second) gIt->second->Release();
			mGameWorldsByRoom.erase(gIt);
		}
	}
}
