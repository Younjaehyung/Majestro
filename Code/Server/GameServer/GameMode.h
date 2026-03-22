#pragma once

class GameMode
{
public:
	virtual ~GameMode() = default;
	virtual void Initialize() = 0;
	virtual void Update(float deltaTime) = 0;

	virtual bool& IsSceneChanging() { return mIsSceneChanging; }
	virtual SceneId GetTargetSceneId() const { return mTargetSceneId; }
public:


protected:
	bool mIsSceneChanging = false; // 씬 전환이 필요한지 여부 (예: 로비에서 게임으로, 게임에서 결과 화면으로 등)
	bool mIsComplete = false;	// 게임 모드 완료 여부 (예: 웨이브 클리어, 게임 승리/패배 등)
	bool mIsFailed = false;		// 게임 모드 실패 여부 (예: 플레이어 사망, 타임아웃 등)
	bool mIsTransitioning = false; // 씬 전환 중 여부 (예: 로비에서 게임으로, 게임에서 결과 화면으로 등)

	SceneId mTargetSceneId; // 전환할 씬의 ID (예: SceneId::Game, SceneId::Result 등)
};

class LobbyGameMode : public GameMode
{
public:
	virtual void Initialize() override;
	virtual void Update(float deltaTime) override;
private:
};

class WaveGameMode : public GameMode
{
public:
	virtual void Initialize() override;
	virtual void Update(float deltaTime) override;
private:

};

class ResultGameMode : public GameMode
{
public: 
	virtual void Initialize() override;
	virtual void Update(float deltaTime) override;

};