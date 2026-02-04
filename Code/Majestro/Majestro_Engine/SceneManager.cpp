#include "pch.h"
#include "SceneManager.h"
#include "Scene.h"
#include "NetSendSystem.h"

void SceneManager::Initialize()
{
	/*mActiveScene = make_shared<Scene>();
	mActiveScene->Initialize();*/
	LoadScene(L"Lobby");
}

void SceneManager::Update(float deltaTime)
{
	if (mHasPendingSceneChange)
	{
		mHasPendingSceneChange = false;
		LoadScene(mPendingSceneName);
		mPendingSceneName.clear();
		if (mPendingGameStart)
		{
			mPendingGameStart = false;
			if (mActiveScene)
			{
				auto world = mActiveScene->GetWorld();
				if (world && world->GetSystemManager())
				{
					auto netSendSystem = world->GetSystemManager()->GetSystem<NetSendSystem>();
					if (netSendSystem)
					{
						if (mHasPendingPlayerType)
						{
							netSendSystem->SetCachedPlayerType(mPendingPlayerType);
							mHasPendingPlayerType = false;
						}
						netSendSystem->QueueGameStart();
					}
				}
			}
		}
	}
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
	if(mActiveScene)
		mActiveScene->Shudown();

	if (sceneName == L"Lobby")
	{
		mActiveScene = make_shared<LobbyScene>();
	}
	else if (sceneName == L"Game")
	{
		mActiveScene = make_shared<GameScene>();
	}
	else
	{
		mActiveScene = make_shared<GameScene>();
	}

	mActiveScene->Initialize();

	

	//mActiveScene = LoadTestScene();

	//mActiveScene->Initialize();

}

void SceneManager::QueueLoadScene(const wstring& sceneName)
{
	mPendingSceneName = sceneName;
	mHasPendingSceneChange = true;
}

void SceneManager::QueueGameStartAfterLoad()
{
	mPendingGameStart = true;
}

void SceneManager::StorePendingPlayerType(uint8 playerType)
{
	 mPendingPlayerType = playerType; 
	 mHasPendingPlayerType = true; 
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

