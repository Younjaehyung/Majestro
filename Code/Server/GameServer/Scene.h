#pragma once
#include "World.h"
#include "InteractableComponent.h"
class SystemManager;
class GameMode;

class Scene : public std::enable_shared_from_this<Scene>
{
public:
	virtual void Initialize();
	virtual void Release() { mWorld->Clear(); }
	virtual void Update(float deltaTime);
	virtual void LoadJsonLevel(const wstring& path);
	virtual void LoadCollisionJson(const wstring& path);
	virtual void LoadNavMesh(const wstring& path);

	Entity SpawnInteractable(World* world,
		uint8 kind,
		const Vec3& position,
		const Vec3& halfExtents,
		float valueA,
		float valueB,
		float cooldown,
		bool  oneShot,
		InteractableTarget targetMask = InteractableTarget_Player,
		SkillType buffType = SkillType::Default);

	Entity SpawnMonsterSpawner(World* world,
		const Vec3& position,
		float interval,
		int32 maxAlive,
		float spawnRadius,
		uint8 triggerRaw = 0,        // SpawnerTrigger::Timed
		uint8 prefabTypeRaw = 5,      // PrefabType::ENEMY
		int32 maxTotal = -1,
		bool  startActive = true);
	virtual const shared_ptr<World>& GetWorld() { return mWorld; }

	void SetGameMode(shared_ptr<GameMode>& gameMode);
	shared_ptr<GameMode>& GetGameMode() { return mGameMode; }
	SceneId GetSceneId() const { return mSceneId; }

	// JSON 의 phaseSpawners 로 정의된 phase 키와 스포너 id 세트 등록.
	void SetPhaseSpawnerSets(std::unordered_map<std::string, std::vector<std::string>> sets);

	// phase 키에 해당하는 스포너만 활성화.
	void ApplyPhaseSpawnerSet(const std::string& phaseKey);
public:
	bool mIsStarted = false;

protected:

	shared_ptr<World>				mWorld = make_shared<World>();
	shared_ptr<GameMode>			mGameMode;
	SceneId							mSceneId;
	wstring							mNavMeshPath;

	// phase 키 , 활성화할 스포너 id 목록 
	std::unordered_map<std::string, std::vector<std::string>> mPhaseSpawnerSets;

	// phaseSpawners 에 한 번이라도 등장한 스포너 id 
	std::unordered_set<std::string> mPhaseManagedSpawnerIds;
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

// 보스전 씬
class ThirdScene : public Scene
{
public:
	virtual void Initialize() override;

};

// 광장(허브) 씬
class PlazaScene : public Scene
{
public:
	virtual void Initialize() override;

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
