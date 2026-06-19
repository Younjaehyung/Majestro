#pragma once
#include "Entity.h"


class Scene;
class GamePhase;

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

	// [디버그] 게임 완료를 기다리지 않고 지정 씬으로 강제 전환
	void DebugForceTransition(SceneId target)
	{
		mTargetSceneId = target;
		mIsSceneChanging = true;
	}
	virtual GameModeType GetType() const = 0;

	virtual void SetScene(shared_ptr<Scene> scene) { mScene = scene; }
	virtual shared_ptr<Scene> GetScene() const { return mScene; }


protected:
	shared_ptr<Scene> mScene; // 게임 모드가 속한 씬에 대한 참조 (예: 로비 씬, 게임 씬 등)

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
	virtual void PostUpdate(float deltaTime) override;

	virtual GameModeType GetType() const override { return GameModeType::Lobby; }
private:
};






class WaveGameMode : public GameMode
{
public:
	using PhaseFactory = std::function<GamePhase*()>;

	virtual void Initialize() override;
	virtual void PreUpdate(float deltaTime) override;
	virtual void PostUpdate(float deltaTime) override;

	virtual GameModeType GetType() const override { return GameModeType::Wave; }

	void TransitionTo(GamePhase* next);
	void AdvancePhase();
	void InsertNextPhase(PhaseFactory factory);

	// 전원 사망(와이프) 시 GameOver(Fail) phase 로 진입한다.
	bool ShouldTriggerGameOver() const;
	void TriggerGameOver();

	virtual float GetGameTime() const { return mGameTime; }

	// 모든 phase 클리어 시 전환할 씬
	void SetCompletionScene(SceneId id) { mCompletionSceneId = id; }

	// 초기 phase 큐를 직접 지정
	void SetInitialPhases(std::deque<PhaseFactory> phases)
	{
		mInitialPhases = std::move(phases);
		mHasCustomPhases = true;
	}

private:
// 런타임
	std::deque<PhaseFactory> mPhaseQueue;
	GamePhase* mCurrentPhase = nullptr;
	uint8 mCurrentPhaseIndex = 0;
	float mGameTime = 0.0f; // 게임 진행 시간
	SceneId mCompletionSceneId = SceneId::MainMenu;

	std::deque<PhaseFactory> mInitialPhases; // SetInitialPhases 로 지정된 초기 큐
	bool mHasCustomPhases = false;

	bool mFailed = false; // 전원 사망으로 GameOver 가 발동됐는지 (중복 진입 방지 + 종료 목적지를 MainMenu 로 강제)

};


class ResultGameMode : public GameMode
{
public: 
	virtual void Initialize() override;
	virtual void PreUpdate(float deltaTime) override;
	virtual void PostUpdate(float deltaTime) override;
	virtual GameModeType GetType() const override { return GameModeType::Result; }

};
