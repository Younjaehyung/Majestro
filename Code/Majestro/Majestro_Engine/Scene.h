#pragma once
#include "World.h"
class SystemManager;
class GameMode;
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

	

	void LoadJsonLevel(const wstring& path);
	void LoadCollisionJson(const wstring& path);
	const shared_ptr<World>& GetWorld() { return mWorld; }


	void SetGameMode(shared_ptr<GameMode>& gameMode);
	shared_ptr<GameMode>& GetGameMode() { return mGameMode; }
	SceneId GetSceneId() const { return mSceneId; }
public:
	bool mIsStarted = false;
protected:

	shared_ptr<World>				mWorld = make_shared<World>();
	shared_ptr<GameMode>			mGameMode;
	SceneId							mSceneId = SceneId::MainMenu;
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

class LoadingScene : public Scene
{
public:
	LoadingScene() { mSceneId = SceneId::Loading; }
	void Initialize() override;
	void Update(float deltaTime) override;


private:
	Entity mLoadingText = NULL_ENTITY;
	Entity mLoadingImage = NULL_ENTITY;
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
