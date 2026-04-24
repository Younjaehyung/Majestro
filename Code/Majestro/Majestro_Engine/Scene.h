#pragma once
#include "World.h"
class SystemManager;
class GameMode;
class UIFeature;

class Scene : public std::enable_shared_from_this<Scene>
{
public:
	Scene();
	virtual ~Scene() = default;

	virtual void Initialize();
	virtual void Release() { Shudown();  mWorld->Clear(); }
	virtual void Update(float deltaTime);
	virtual void Render();
	virtual void Shudown();

	
	virtual void Enter() { mIsStarted = true; }
	virtual void Exit() { mIsStarted = false; }

	// 맵 데이터 fbx로드, 맵 데이터 json로드
	void LoadJsonLevelFBX(const wstring& path);
	void LoadJsonLevelData(const wstring& path);

	// 맵 데이터 전체 로드
	void LoadJsonLevel(const wstring& path);
	void LoadCollisionJson(const wstring& path);
	


	void SetGameMode(shared_ptr<GameMode>& gameMode);
	shared_ptr<GameMode>& GetGameMode() { return mGameMode; }
	const shared_ptr<World>& GetWorld() { return mWorld; }
	SceneId GetSceneId() const { return mSceneId; }

public:
	bool mIsStarted = false;
	std::vector< shared_ptr<UIFeature>> mUIFeatures;
protected:
	wstring							mMapPath;			// 로딩할 맵 데이터 경로
	shared_ptr<World>				mWorld = make_shared<World>();
	shared_ptr<GameMode>			mGameMode;
	SceneId							mSceneId = SceneId::MainMenu;
};

class LoadingScene		// 로딩전용씬
{
public:
	LoadingScene() = default;
	
	virtual void Initialize();
	virtual void Release() { Shudown();}
	virtual void Update(float deltaTime);
	virtual void Render();
	virtual void Shudown();

	bool LoadScene(SceneId id);
	void ProcessTask();              // 1개 실행
	bool IsLoadDone() const { return mLoadTasks.empty(); }
	float GetProgress() const        // 0.0 ~ 1.0
	{
		if (mTotalTaskCount == 0) return 1.f;
		int32 done = mTotalTaskCount - (int32)mLoadTasks.size();
		return (float)done / (float)mTotalTaskCount;
	}

public:
	std::queue<std::function<void()>> mLoadTasks;
	int32			mTotalTaskCount = 0;       // 등록 시점에 기록
	SceneId			mTargetSceneId;
	std::vector< shared_ptr<UIFeature>> mUIFeatures;
	bool mIsStarted = false;
protected:
	
	Entity	mProgressBar;
	float mProgressBarMaxWidth = 2500.f;
	/*std::array<,(uint8)SceneId::End>			mLoadingSceneId;*/
	shared_ptr<World>				mWorld = make_shared<World>();
};


class MainMenuScene : public Scene
{
public:
	MainMenuScene() { mSceneId = SceneId::MainMenu; }
	void Initialize() override;
	
	
};

class LobbyScene : public Scene
{
public:
	LobbyScene() { mSceneId = SceneId::Lobby; }
	void Initialize() override;
};

// INGAME SCENE 

class FirstScene : public Scene
{
public:
	FirstScene() { mSceneId = SceneId::FirstGame; }
	virtual void Initialize() override;

};

class SecondScene : public Scene
{
public:
	SecondScene() { mSceneId = SceneId::SecondGame; }
	virtual void Initialize();

};

// RESULT SCENE

class VictoryScene : public Scene
{
public:
	VictoryScene() { mSceneId = SceneId::VGame; }
	virtual void Initialize() override;

};

class LoseScene : public Scene
{
public:
	LoseScene() { mSceneId = SceneId::LGame; }
	virtual void Initialize() override;

};
