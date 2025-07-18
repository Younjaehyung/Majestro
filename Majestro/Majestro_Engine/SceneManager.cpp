#include "pch.h"
#include "SceneManager.h"
#include "Scene.h"

void SceneManager::Initialize()
{
}

void SceneManager::Update(float deltaTime)
{
	if (mActiveScene == nullptr)
		return;

	mActiveScene->Update(deltaTime);

}

void SceneManager::Render()
{
	mActiveScene->Render();
}

void SceneManager::LoadScene(wstring sceneName)
{
	// TODO : 기존 Scene 정리
	// TODO : 파일에서 Scene 정보 로드

	//mActiveScene = LoadTestScene();

	//mActiveScene->Initialize();

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

