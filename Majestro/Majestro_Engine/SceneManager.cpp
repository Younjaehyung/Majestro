#include "pch.h"
#include "SceneManager.h"
#include "Scene.h"

void SceneManager::Initialize()
{
}

void SceneManager::Update(float deltaTime)
{
	if (_activeScene == nullptr)
		return;

	_activeScene->Update(deltaTime);
	_activeScene->LateUpdate(deltaTime);
	_activeScene->FinalUpdate(deltaTime);
}

void SceneManager::LoadScene(wstring sceneName)
{
	// TODO : 기존 Scene 정리
	// TODO : 파일에서 Scene 정보 로드

	_activeScene = LoadTestScene();

	_activeScene->Awake();
	_activeScene->Start();
}

void SceneManager::Render()
{
	if (_activeScene)
		_activeScene->Render();


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

