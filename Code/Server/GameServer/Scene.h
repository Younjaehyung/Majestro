#pragma once
#include "World.h"
class SystemManager;
class GameMode;

class Scene
{
public:
	virtual void Initialize();
	virtual void Release() { mWorld->Clear(); }
	virtual void Update(float deltaTime);
	virtual void LoadJsonLevel(const wstring& path);
	virtual void LoadCollisionJson(const wstring& path);
	Entity SpawnInteractable(World* world,
		uint8 kind,
		const Vec3& position,
		const Vec3& halfExtents,
		float valueA,
		float valueB,
		float cooldown,
		bool  oneShot,
		SkillType buffType = SkillType::Default);
	virtual const shared_ptr<World>& GetWorld() { return mWorld; }

	void SetGameMode(shared_ptr<GameMode>& gameMode) { mGameMode = gameMode; }
	shared_ptr<GameMode>& GetGameMode() { return mGameMode; }
	SceneId GetSceneId() const { return mSceneId; }
public:
	bool mIsStarted = false;

protected:

	shared_ptr<World>				mWorld = make_shared<World>();
	shared_ptr<GameMode>			mGameMode;
	SceneId							mSceneId;
};

// INGAME SCENE 

class FirstScene : public Scene
{
public:
	virtual void Initialize() override;

};

class SecondScene : public Scene
{
public:
	virtual void Initialize();

};

// LOBBY SCENE

class LobbyScene : public Scene
{
public:
	virtual void Initialize() override;

};
class LoadingScene : public Scene
{
public:
	virtual void Initialize() override;

};

// RESULT SCENE

class VictoryScene : public Scene
{
public:
	virtual void Initialize() override;

};

class LoseScene : public Scene
{
public:
	virtual void Initialize() override;

};
