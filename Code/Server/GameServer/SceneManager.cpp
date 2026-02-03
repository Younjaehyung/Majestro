#include "pch.h"
#include "SceneManager.h"
#include "Scene.h"
#include "ServerCore.h"

void SceneManager::Initialize()
{
	//mActiveScene = make_shared<Scene>();
	//mActiveScene->Initialize();

	//mScenesBySession.clear();
	mLobbyScenesBySession.clear();
	mSceneBySession.clear();
	mGameScene.reset();
}

void SceneManager::Update(float deltaTime)
{
	/*if (mActiveScene == nullptr)
		return;
	mActiveScene->Update(deltaTime);*/

	//for (const auto& [sessionId, scene] : mScenesBySession)
	for (const auto& [sessionId, scene] : mLobbyScenesBySession)
	{
		if (scene == nullptr)
			continue;

		scene->Update(deltaTime);
	}

	if (mGameScene)
	{
		mGameScene->Update(deltaTime);
	}

}


void SceneManager::LoadScene(wstring sceneName)
{
	// TODO : 기존 Scene 정리
	// TODO : 파일에서 Scene 정보 로드

	//mActiveScene = LoadTestScene();

	//mActiveScene->Initialize();

}

void SceneManager::InitializeSession(uint64 sessionId)
{
	//if (mScenesBySession.find(sessionId) != mScenesBySession.end())
	if (mLobbyScenesBySession.find(sessionId) != mLobbyScenesBySession.end())
		return;

	auto scene = make_shared<Scene>();
	scene->Initialize();
	//mScenesBySession.emplace(sessionId, std::move(scene));
	mLobbyScenesBySession.emplace(sessionId, std::move(scene));
	mSceneBySession[sessionId] = SceneId::Lobby;
}

void SceneManager::RemoveSession(uint64 sessionId)
{
	//mScenesBySession.erase(sessionId);
	mLobbyScenesBySession.erase(sessionId);
	mSceneBySession.erase(sessionId);
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

	if (sceneState == SceneId::Game)
	{
		return mGameScene;
	}

	auto findIt = mLobbyScenesBySession.find(sessionId);
	if (findIt == mLobbyScenesBySession.end())
		return nullptr;

	return findIt->second;
}

bool SceneManager::EnqueueCommand(const InputCommand& command)
{
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
	const bool isApproved = IsSceneChangeAllowed(currentScene, requestedScene);
	if (isApproved)
	{
		mSceneBySession[command.SessionId] = requestedScene;
		if (requestedScene == SceneId::Game && !mGameScene)
		{
			auto scene = make_shared<Scene>();
			scene->Initialize();
			mGameScene = std::move(scene);
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

	return true;
}

bool SceneManager::IsSceneChangeAllowed(SceneId currentScene, SceneId requestedScene) const
{
	if (currentScene == requestedScene)
		return false;

	switch (currentScene)
	{
	case SceneId::Lobby:
		return requestedScene == SceneId::Game;
	case SceneId::Game:
		return requestedScene == SceneId::Lobby;
	default:
		return false;
	}
}

SceneId SceneManager::GetOrCreateSceneState(uint64 sessionId)
{
	auto findIt = mSceneBySession.find(sessionId);
	if (findIt != mSceneBySession.end())
		return findIt->second;

	InitializeSession(sessionId);
	return SceneId::Lobby;
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

