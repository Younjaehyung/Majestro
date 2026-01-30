#include "pch.h"
#include "SceneManager.h"
#include "Scene.h"

void SceneManager::Initialize()
{
	//mActiveScene = make_shared<Scene>();
	//mActiveScene->Initialize();

	mScenesBySession.clear();
}

void SceneManager::Update(float deltaTime)
{
	/*if (mActiveScene == nullptr)
		return;
	mActiveScene->Update(deltaTime);*/

	for (const auto& [sessionId, scene] : mScenesBySession)
	{
		if (scene == nullptr)
			continue;

		scene->Update(deltaTime);
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
	if (mScenesBySession.find(sessionId) != mScenesBySession.end())
		return;

	auto scene = make_shared<Scene>();
	scene->Initialize();
	mScenesBySession.emplace(sessionId, std::move(scene));
}

void SceneManager::RemoveSession(uint64 sessionId)
{
	mScenesBySession.erase(sessionId);
}

void SceneManager::LoadScene(uint64 sessionId, wstring sceneName)
{
	// TODO : 기존 Scene 정리
	// TODO : 파일에서 Scene 정보 로드

	auto scene = make_shared<Scene>();
	scene->Initialize();
	mScenesBySession[sessionId] = std::move(scene);
}

shared_ptr<Scene> SceneManager::GetScene(uint64 sessionId) const
{
	auto findIt = mScenesBySession.find(sessionId);
	if (findIt == mScenesBySession.end())
		return nullptr;

	return findIt->second;
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

