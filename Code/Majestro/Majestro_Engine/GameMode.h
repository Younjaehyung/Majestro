#pragma once
class Scene;

enum class GameModeType : uint8
{
	Lobby = 0,
	Wave,
	Result,
};

class GameMode
{
public:
	virtual ~GameMode() = default;
	virtual void Initialize() = 0;
	virtual void PreUpdate(float deltaTime) {}   // 입력 감지, 이벤트 발행
	virtual void PostUpdate(float deltaTime) {}  // 시뮬레이션 결과 판정

	virtual bool& IsSceneChanging() { return mIsSceneChanging; }
	virtual SceneId GetTargetSceneId() const { return mTargetSceneId; }
	//virtual GameModeType GetType() const = 0;

	virtual void SetScene(shared_ptr<Scene> scene) { mScene = scene; }
	virtual shared_ptr<Scene> GetScene() const { return mScene; }
	//virtual Entity GetGameRuleEntity() const { return mGameRuleEntity; }

public:

	SceneId mTargetSceneId{}; // 전환할 씬의 ID (예: SceneId::Game, SceneId::Result 등)
protected:
	shared_ptr<Scene> mScene; // 게임 모드가 속한 씬에 대한 참조 (예: 로비 씬, 게임 씬 등)

	bool mIsSceneChanging = false; // 씬 전환이 필요한지 여부 (예: 로비에서 게임으로, 게임에서 결과 화면으로 등)
	bool mIsComplete = false;	// 게임 모드 완료 여부 (예: 웨이브 클리어, 게임 승리/패배 등)
	bool mIsFailed = false;		// 게임 모드 실패 여부 (예: 플레이어 사망, 타임아웃 등)
	bool mIsTransitioning = false; // 씬 전환 중 여부 (예: 로비에서 게임으로, 게임에서 결과 화면으로 등)


};

class MenuGameMode : public GameMode
{
public:
	virtual void Initialize() override;
	virtual void PreUpdate(float deltaTime) override;


private:
};

class LobbyGameMode : public GameMode
{
public:
	virtual void Initialize() override;
	virtual void PreUpdate(float deltaTime) override;
private:
};

class WaveGameMode : public GameMode
{
public:
	virtual void Initialize() override;
	virtual void PreUpdate(float deltaTime) override;
	virtual void PostUpdate(float deltaTime) override;

	//virtual GameModeType GetType() const override { return GameModeType::Wave; }


};

class ResultGameMode : public GameMode
{
public:
	virtual void Initialize() override;

};