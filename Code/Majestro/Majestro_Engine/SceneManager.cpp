#include "pch.h"
#include "SceneManager.h"
#include "RenderManager.h"
#include "Scene.h"
#include "NetSendSystem.h"
#include "Engine.h"
#include "InputManager.h"
#include "GameMode.h"

void SceneCommandProcessor::Submit(SceneCommand cmd)
{

	if (mPending.IsValid())
	{
		std::cerr << "Warning: Overwriting pending scene command. Previous command will be lost." << std::endl;
	}
	mPending = std::move(cmd);
}

// 매 프레임 StartRender() 이전에 호출
// true 반환 = 씬 전환이 실행됨
bool SceneCommandProcessor::Process()
{
	if (!mPending.IsValid())
		return false;

	SceneCommand cmd = std::move(mPending);
	mPending = {};

	switch (cmd.type)
	{
	case SceneCommandType::ImmediateLoad:
		ExecuteLoadScene(cmd);
		break;

	case SceneCommandType::LoadingThenScene:
		ExecuteLoadingThenScene(cmd);
		break;

	default:
		break;
	}

	return true;
}

// 씬 즉시 전환
void SceneCommandProcessor::ExecuteLoadScene(const SceneCommand& cmd)
{
	mOwner->LoadScene(cmd.targetScene);
}


// 로딩창에서 로딩 후 씬 전환
void SceneCommandProcessor::ExecuteLoadingThenScene(const SceneCommand& cmd)
{
	mOwner->ReleaseScene();
	mOwner->SetLoadingMessage(cmd.loadingMessage);
	mOwner->mIsLoading = mOwner->mLoadingScene->LoadScene(cmd.targetScene);

}



//////////////////////////////////////////////////////////
/// SceneManager
//////////////////////////////////////////////////////////

void SceneManager::Initialize()
{
	FactoryScene();
	LoadScene(SceneId::MainMenu);  // 첫 씬은 즉시 로드
}

void SceneManager::FactoryScene()
{
	{
		mLoadingScene = make_shared<LoadingScene>();
		mLoadingScene->Initialize();


	}


	{	// MainMenuScene
		shared_ptr<Scene> mainMenuScene = make_shared<MainMenuScene>();
		shared_ptr<GameMode> gameMode = make_shared<MenuGameMode>();
		mainMenuScene->SetGameMode(gameMode);
		mGameScenes[(uint8)SceneId::MainMenu] = mainMenuScene;
	}
	{	// LOBBYSCENE
		shared_ptr<Scene> lobbyScene = make_shared<LobbyScene>();
		shared_ptr<GameMode> gameMode = make_shared<LobbyGameMode>();

		//lobbyScene->Initialize();
		lobbyScene->SetGameMode(gameMode);
		mGameScenes[(uint8)SceneId::Lobby]=lobbyScene;
	}



	{	// GAMESCENE
		shared_ptr<Scene> firstScene = make_shared<FirstScene>();
		shared_ptr<GameMode> gameMode = make_shared<WaveGameMode>();
		//firstScene->Initialize();
		firstScene->SetGameMode(gameMode);
		mGameScenes[(uint8)SceneId::FirstGame] = firstScene;
		
	}
	{

		shared_ptr<Scene> secondScene = make_shared<SecondScene>();
		shared_ptr<GameMode> gameMode = make_shared<WaveGameMode>();
		//secondScene->Initialize();
		secondScene->SetGameMode(gameMode);
		mGameScenes[(uint8)SceneId::SecondGame] = secondScene;

	}



	{	// RESULTSCENE
		shared_ptr<Scene> victoryScene = make_shared<VictoryScene>();
		shared_ptr<GameMode> gameMode = make_shared<ResultGameMode>();
		//victoryScene->Initialize();
		victoryScene->SetGameMode(gameMode);
		mGameScenes[(uint8)SceneId::VGame] = victoryScene;
	}
	{
		shared_ptr<Scene> loseScene = make_shared<LoseScene>();
		shared_ptr<GameMode> gameMode = make_shared<ResultGameMode>();
		//loseScene->Initialize();
		loseScene->SetGameMode(gameMode);
		mGameScenes[(uint8)SceneId::LGame] = loseScene;
	}



}

void SceneManager::CheckGameModeSceneRequest()
{
	if (!mActiveScene)
		return;

	auto& gameMode = mActiveScene->GetGameMode();
	if (!gameMode || !gameMode->IsSceneChanging())
		return;

	RequestScene(gameMode->GetTargetSceneId());
	gameMode->IsSceneChanging() = false;
}


void SceneManager::Update(float deltaTime)
{


	if (mIsLoading)
	{
		if (!mLoadingScene->IsLoadDone())
			mLoadingScene->ProcessTask();   // 태스크 1개 실행
		else
		{
			mLoadingScene->Release();

			LoadScene(mLoadingScene->mTargetSceneId);
			mIsLoading = false;
			return;
		}
		mLoadingScene->Update(deltaTime);   // 프로그레스바 갱신
		return;
	}


	if (mActiveScene == nullptr)
		return;
	
	mActiveScene->Update(deltaTime);
	CheckGameModeSceneRequest();
}

void SceneManager::Render()
{
	if (mIsLoading)
	{
		mLoadingScene->Render();
		return;
	}
	mActiveScene->Render();
}

void SceneManager::LoadScene(SceneId id)
{
	mActiveScene = mGameScenes[(size_t)id];
	mActiveScene->Initialize();
	mActiveScene->Enter();
}

void SceneManager::ReleaseScene()
{
	if (mActiveScene) {
		mActiveScene->Exit();
		mActiveScene->Release();
	}
}

void SceneManager::SetLoadingMessage(const wstring& loadingMessage)
{
	mLoadingMessage = loadingMessage;
}


void SceneManager::SetLayerName(uint8 index, const wstring& name)
{
	// 기존 데이터 삭제
	const wstring& prevName = mLayerNames[index];
	mLayerIndex.erase(prevName);

	mLayerNames[index] = name;
	mLayerIndex[name] = index;
}

uint8 SceneManager::LayerNameToIndex(const wstring& name)
{
	auto findIt = mLayerIndex.find(name);
	if (findIt == mLayerIndex.end())
		return 0;

	return findIt->second;
}

