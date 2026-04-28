#pragma once
#include "Entity.h"

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
	virtual void SendSceneState(float deltaTime, const std::vector<uint32>& recipients) {} // 씬 상태를 클라이언트에 전송 (예: 웨이브 진행 상황, 게임 결과 등)

	virtual bool& IsSceneChanging() { return mIsSceneChanging; }
	virtual SceneId GetTargetSceneId() const { return mTargetSceneId; }
	virtual GameModeType GetType() const = 0;

	virtual void SetScene(shared_ptr<Scene> scene) { mScene = scene; }
public:


protected:
	shared_ptr<Scene> mScene; // 게임 모드가 속한 씬에 대한 참조 (예: 로비 씬, 게임 씬 등)

	bool mIsSceneChanging = false; // 씬 전환이 필요한지 여부 (예: 로비에서 게임으로, 게임에서 결과 화면으로 등)
	bool mIsComplete = false;	// 게임 모드 완료 여부 (예: 웨이브 클리어, 게임 승리/패배 등)
	bool mIsFailed = false;		// 게임 모드 실패 여부 (예: 플레이어 사망, 타임아웃 등)
	bool mIsTransitioning = false; // 씬 전환 중 여부 (예: 로비에서 게임으로, 게임에서 결과 화면으로 등)

	SceneId mTargetSceneId; // 전환할 씬의 ID (예: SceneId::Game, SceneId::Result 등)

	SendRequest mSendReq;
};

class LobbyGameMode : public GameMode
{
public:
	virtual void Initialize() override;
	virtual void PostUpdate(float deltaTime) override;
	virtual void SendSceneState(float deltaTime, const std::vector<uint32>& recipients) override;

	virtual GameModeType GetType() const override { return GameModeType::Lobby; }
private:
};



class WaveGameMode : public GameMode
{
public:
	virtual void Initialize() override;
	virtual void PreUpdate(float deltaTime) override;
	virtual void PostUpdate(float deltaTime) override;
	virtual void SendSceneState(float deltaTime, const std::vector<uint32>& recipients) override;

	virtual GameModeType GetType() const override { return GameModeType::Wave; }

public:
	float GetGameTime() const { return mGameTime; }
	int GetCurrentWave() const { return mWave; }
	float GetWaveTime() const { return mWaveTime; }
	float GetWaveInterval() const { return mWaveInterval; }
	int GetWaveCheckPoint() const { return mWaveCheckPoint; }
	int GetPlayerNum() const { return mPlayerNum; }
	int GetEnemyNum() const { return mEnemyNum; }

private:

	// 상수

	static constexpr int mMaxWaves = 3; // 최대 웨이브 수
	
	static constexpr int mMaxWaveCheckPoint = 3; // 최대 웨이브 체크포인트 수
	static constexpr float mWaveCheckPointTime = 10.f; // 웨이브 체크포인트별 시간

	static constexpr float mMaxWaveInterval = 3.f; // 웨이브 점령 감소 간격 시작 (초)
	static constexpr float mMaxWaveTime = 30.f; // 최대 웨이브 수

	std::array<std::vector<Entity>, mMaxWaves> mEnemeySpawners; // 웨이브별 적 스포너 엔티티 리스트
	std::array<Entity, mMaxWaves> mPlayerSpawners;				// 웨이브별 플레이어 스포너 엔티티 리스트
	std::array<Entity, mMaxWaves> mConquestPointRect;				// 플레이어가 들어가야 하는 체크포인트 영역

	// 런타임

	float mGameTime = 0.0f; // 게임 진행 시간

	int mWaveCheckPoint = 0; // 현재 웨이브 체크포인트 번호 (0부터 시작)
	int mWave = 1;          // 현재 웨이브 번호 (1부터 시작)

	float mWaveInterval = 0.0f; // 웨이브 점령 감소 간격 (초)
	float mWaveTime = 0.0f; // 웨이브 점령 시간

	int mPlayerNum = 0; // 플레이어 수
	int mEnemyNum = 0; // 적 수



};


class ResultGameMode : public GameMode
{
public: 
	virtual void Initialize() override;
	virtual void PreUpdate(float deltaTime) override;
	virtual void PostUpdate(float deltaTime) override;
	virtual void SendSceneState(float deltaTime, const std::vector<uint32>& recipients) override;
	virtual GameModeType GetType() const override { return GameModeType::Result; }

};