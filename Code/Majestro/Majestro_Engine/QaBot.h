#pragma once
#include <cstdarg>
#include <cstdio>
#include <map>
#include <string>
#include "InputManager.h"

class World;

// 실행 인자 -qa 로 켜는 무인 QA 플레이어.
//   사람 대신 입력을 공급해 메뉴 -> 로비 -> 광장 -> 스테이지(순서대로) 를 진행하고,
//   그 과정에서 관찰한 이상 징후를 작업 폴더의 qa_log.txt 에 남긴다.
//   메뉴/씬 전환은 UI 버튼이 발행하는 것과 같은 이벤트를 발행하고, 인게임 조작은 키·마우스 입력으로만 한다.
//
//   씬을 넘어 유지되는 단일 상태라 Chat 과 같은 이유로 싱글턴이다. 배선은 Engine::Initialize 가 한다.
//
//   인자:  -qastages=N       진행할 스테이지 수 (기본 전체)
//          -qafirst=N        N번째 스테이지부터 시작 (기본 1)
//          -qachar=N         캐릭터 선택: 1 Rudwig, 2 Ibanix, 3 Fanthor (기본: 로비 기본값)
//          -qagoto=x,y,z     첫 레벨에서 이 좌표(cm)까지 먼저 걸어간 뒤 평소 행동 시작
//          -qaphasetimeout=S 한 페이즈가 이 시간 넘게 안 끝나면 그 스테이지를 포기하고 다음으로 (기본 300)
//          -qatimeout=S      전체 실행 제한 (기본 1800)
class QaBot
{
public:
	static bool Requested();
	static QaBot& Get();

	// InputManager 입력 소스. 매 프레임 1회 호출되며 판단과 입력 생성을 함께 한다.
	void ProduceInput(InputFrame& frame);

private:
	QaBot();

	void OnSceneChanged(SceneId sceneId);
	void TickMainMenu(World* world, bool retry);
	void TickLobby(World* world, bool retry);
	void TickPlaza(World* world, InputFrame& frame, float dt, bool retry);
	void TickLevel(World* world, InputFrame& frame, float dt, bool retry);

	void RequestScene(World* world, SceneId target, bool retry);
	// 목표 지점으로 조향한다. 정면으로 정렬되면 전진 키를 누르고, 막히면 점프·옆걸음으로 빠져나온다.
	void Steer(World* world, InputFrame& frame, float dt, const Vec3& position, const Vec3& target, bool move);
	// 스테이지 NavMesh 를 연다 (서버가 쓰는 것과 같은 파일). 없으면 직선 주행으로 남는다.
	void LoadNav(SceneId sceneId);
	// target 까지의 경로에서 다음으로 향할 지점. 경로가 없으면 target 그대로.
	Vec3 PathStep(const Vec3& position, const Vec3& target);
	// 지금 스테이지를 포기하고 다음 스테이지로 넘어간다 (마지막이면 종료).
	void AbandonStage(World* world);
	void Finish();

	void Info(const char* format, ...);
	void Warn(const char* key, const char* format, ...);
	void Fail(const char* key, const char* format, ...);
	void Write(const char* level, const char* format, va_list args);

private:
	FILE* mLog = nullptr;
	std::map<std::string, int> mFailCounts;
	std::map<std::string, int> mWarnCounts;

	float mTime = 0.f;
	float mSceneTime = 0.f;
	float mRetryTimer = 0.f;
	SceneId mScene = SceneId::End;
	bool mFinished = false;

	// 씬 전환 요청
	SceneId mRequestedScene = SceneId::End;
	float mRequestElapsed = 0.f;

	// 스테이지 진행
	int mStageLimit = 0;
	int mStageIndex = 0;
	int mFirstStage = 0;
	int mCharacter = 0;		// -qachar: 1 Rudwig, 2 Ibanix, 3 Fanthor, 0 이면 로비 기본값 그대로
	bool mGotoPending = false;	// -qagoto: 첫 레벨에서 이 좌표까지 먼저 걸어간다
	Vec3 mGoto{};
	int mStagesCleared = 0;
	bool mStageCleared = false;
	uint8 mPhase = 0;
	float mPhaseTime = 0.f;
	float mPhaseTimeout = 300.f;
	float mRunTimeout = 1800.f;

	// 광장 이동 점검
	bool mMoveTestDone = false;
	bool mMoveTestStarted = false;
	float mMoveTestTime = 0.f;
	Vec3 mMoveTestStart{};

	// 플레이어 관찰
	bool mHasSpawnHeight = false;
	float mSpawnHeight = 0.f;
	bool mWasDead = false;
	int mDeaths = 0;

	// 끼임 감지
	float mStuckWindow = 0.f;
	Vec3 mStuckWindowStart{};
	float mStuckTime = 0.f;
	float mUnstickTimer = 0.f;
	bool mUnstickLeft = false;

	// 공격 유효성
	Entity mAttackTarget;
	int32 mAttackTargetHp = 0;
	float mAttackTime = 0.f;
	int mTargetsDown = 0;
	float mStatusTimer = 0.f;

	// 길찾기
	std::shared_ptr<class NavMesh> mNavMesh;
	dtNavMeshQuery* mNavQuery = nullptr;
	float mPathTimer = 0.f;
	Vec3 mPathNext{};
	float mStageStuckTime = 0.f;	// 이 스테이지에서 끼여 있던 누적 시간

	// 리듬
	float mLastSongPos = 0.f;
	int mJudgements[3] = {};		// 서버 확정 판정 수: Miss, Good, Perfect
	int mCrits = 0;					// 적에게 들어간 치명타 수
	int mSkillsPressed = 0;
	int mUltimates = 0;
	int mLastRhythmPoints = 0;
	bool mActionWasDown = false;
	float mRhythmChangeTimer = 0.f;
	float mRhythmCheckTimer = -1.f;	// 0 이상이면 리듬 전환 결과 확인 대기 중
	uint8 mRhythmExpected = 0;
	int mRhythmChanges = 0;
	bool mRhythmClickPending = false;	// 우클릭 직후 한 프레임: 클라이언트가 클릭을 받았는지 확인 대기
	float mUltimateHold = 0.f;			// 0 보다 크면 궁극기 진행 중으로 보고 Q 와 공격 무효 판정을 쉰다
	float mHealHold = 0.f;				// 0 보다 크면 회복팩을 찾지 않는다
	bool mHealSeeking = false;
};
