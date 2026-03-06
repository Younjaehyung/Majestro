#include "pch.h"
#include "SceneManager.h"
#include "RenderManager.h"
#include "Scene.h"
#include "NetSendSystem.h"
#include "Engine.h"
#include "InputManager.h"

void SceneManager::Initialize()
{
	/*mActiveScene = make_shared<Scene>();
	mActiveScene->Initialize();*/
	QueueLoadingScene(L"시작 씬 로딩 중...", LoadingVisualType::Startup);
	QueueLoadScene(L"Lobby");
}

void SceneManager::ProcessPendingSceneChanges()
{
	// StartRender() 이전에 호출 — 커맨드 리스트가 열리기 전에 씬 전환 처리
	// (EffectPass::Initialize() → Effekseer::Effect::Create() GPU 업로드가
	//  BeginCommandList 이후에 실행되면 커맨드 얼로케이터 동기화 에러 발생)
	if (mHasPendingLoadingScene)
	{
		mHasPendingLoadingScene = false;
		if (mActiveSceneName != L"Loading")
		{
			LoadScene(L"Loading");
			return;
		}
	}
	if (mHasPendingSceneChange)
	{
		mHasPendingSceneChange = false;
		LoadScene(mPendingSceneName);
		mPendingSceneName.clear();
		mIsLoading = false;
		mLoadingMessage.clear();
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

	// 씬 전환 전 GPU 작업 완전 완료 대기
	// (ProcessPendingSceneChanges → StartRender 이전 호출 보장)
	GRAPHICS_CMD_QUEUE->WaitForGpuComplete();

	if(mActiveScene)
		mActiveScene->Shudown();

	if (sceneName == L"Lobby")
	{
		mActiveScene = make_shared<LobbyScene>();
	}
	else if (sceneName == L"Loading")
	{
		mActiveScene = make_shared<LoadingScene>();
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
	mActiveSceneName = sceneName;

	INPUT.SetForceMouseLook(sceneName == L"Game");

	

	//mActiveScene = LoadTestScene();

	//mActiveScene->Initialize();

}

void SceneManager::QueueLoadScene(const wstring& sceneName)
{
	mPendingSceneName = sceneName;
	mHasPendingSceneChange = true;
}

void SceneManager::QueueLoadingScene(const wstring& loadingMessage, LoadingVisualType visualType)
{
	mLoadingMessage = loadingMessage;
	mLoadingVisualType = visualType;
	mIsLoading = true;
	mHasPendingLoadingScene = true;
}

void SceneManager::SetLoadingMessage(const wstring& loadingMessage)
{
	mLoadingMessage = loadingMessage;
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

