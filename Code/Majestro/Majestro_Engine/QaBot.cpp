#include "pch.h"
#include "QaBot.h"
#include "Engine.h"
#include "SceneManager.h"
#include "Scene.h"
#include "Timer.h"
#include "Network.h"
#include "GameEvents.h"
#include "MathUtils.h"
#include "CameraView.h"
#include "TransformComponent.h"
#include "TagComponent.h"
#include "PlayerComponent.h"
#include "MovementComponent.h"
#include "EnemyComponent.h"
#include "HealthComponent.h"
#include "GameRuleComponent.h"
#include "LobbyRoomListComponent.h"
#include "LobbyRoomStateComponent.h"
#include "MainMenuController.h"
#include "BeatSystem.h"
#include "NavMeshLoader.h"

namespace
{
	constexpr float kBeatLeadSec = 0.03f;			// 박자 이만큼 전부터 누른다 (Perfect 창 0.08 안)
	constexpr float kBeatHoldSec = 0.10f;			// 박자 뒤 이만큼까지 누르고 있다가 뗀다
	constexpr int64 kSkill1EveryBeats = 8;			// 이 박자마다 공격 대신 E 스킬
	constexpr int64 kSkill2EveryBeats = 16;			// 이 박자마다 공격 대신 SHIFT 스킬
	constexpr float kRhythmChangeIntervalSec = 15.f;	// 리듬 전환(우클릭) 간격
	constexpr float kRhythmChangeCheckSec = kMusicLoopSeconds + 2.5f;	// 서버는 다음 16박자 루프 경계(최대 7.5초 뒤)에 전환을 적용한다
	// 궁극기를 쓴 뒤 이 시간 동안은 Q 를 다시 누르지 않는다.
	// Ibanix 는 궁극기 피해로 게이지가 다시 차고, 궁극기 중 Q 는 취소라서 누르면 취소-재발동만 반복한다.
	constexpr float kUltimateHoldSec = 6.f;
	constexpr float kUltimateBusySec = 5.f;			// 그중 실제로 궁극기에 묶여 있는 시간 (도입 1초 + 가장 긴 Ibanix 8박자)
	constexpr float kBlockerDistance = 400.f;		// 점령 중에도 이 안의 적은 먼저 상대한다 (cm)
	constexpr float kPathReplanSec = 0.3f;
	constexpr float kWaypointReachDistance = 80.f;	// 경로 지점에 이만큼 붙으면 다음 지점으로 (cm)
	constexpr float kStuckAbandonSec = 90.f;		// 한 스테이지에서 끼임 누적이 이만큼이면 포기

	// 서버 ResourceManager 가 스테이지별로 여는 NavMesh 와 같은 파일
	const wchar_t* NavFile(SceneId id)
	{
		switch (id)
		{
		case SceneId::FirstGame:  return L"all_tiles_navmesh.bin";
		case SceneId::SecondGame: return L"MapDesert_navmesh.bin";
		case SceneId::ThirdGame:  return L"Map003_navmesh.bin";
		case SceneId::FourthGame: return L"MapDragon_navmesh.bin";
		default:                  return nullptr;
		}
	}

	constexpr float kRetrySec = 2.f;				// 같은 요청을 다시 보내는 간격
	constexpr float kSceneChangeTimeoutSec = 40.f;	// 씬 전환 요청 후 이 시간 안에 안 바뀌면 실패
	constexpr float kHitchSec = 0.2f;				// 한 프레임이 이보다 길면 끊김으로 기록
	constexpr float kSettleSec = 5.f;				// 씬 진입 직후 로딩 여파를 판정에서 빼는 시간

	constexpr float kMoveTestDistance = 100.f;		// 광장 이동 점검: 이만큼 움직이면 통과 (cm)
	constexpr float kMoveTestTimeoutSec = 10.f;

	constexpr float kObjectiveHoldDistance = 250.f;	// 목표 지점에 이만큼 붙으면 멈춘다 (cm)
	constexpr float kObjectiveLeash = 1100.f;		// 목표가 있을 때 적을 쫓아 나가는 한계 (cm). 서버 호위 판정 반경 1000 보다 넓어야 트럭을 막는 적을 치운다
	constexpr float kStatusIntervalSec = 20.f;		// 진행 상황 기록 간격
	constexpr float kEngageDistance = 1500.f;		// 이 안의 적은 조준하고 공격한다 (cm)
	constexpr float kMeleeDistance = 220.f;			// 적에게 이만큼 붙으면 멈춘다 (cm)
	constexpr float kAimToleranceDeg = 20.f;		// 조준 오차가 이 안이면 공격 입력
	constexpr float kMoveToleranceDeg = 60.f;		// 진행 방향 오차가 이 안이면 전진 입력
	constexpr long  kMaxMousePixels = 40;			// 한 프레임에 주입하는 마우스 이동 한계

	constexpr float kStuckWindowSec = 2.f;			// 이 시간 동안
	constexpr float kStuckDistance = 30.f;			//   이만큼도 못 움직이면 끼인 것으로 본다 (cm)
	constexpr float kStuckFailSec = 12.f;			// 끼임이 이만큼 이어지면 실패
	constexpr float kUnstickSec = 1.f;				// 점프 + 옆걸음 시간

	constexpr float kFallDistance = 2000.f;			// 스폰 높이에서 이만큼 떨어지면 맵 밖 낙하 (cm)
	constexpr float kAttackIneffectiveSec = 12.f;	// 붙어서 이만큼 때렸는데 체력이 안 줄면 실패
	constexpr int   kMaxLogsPerKey = 3;				// 같은 문제는 이 횟수까지만 기록하고 나머지는 집계

	// 실행 인자에 정확히 이 토큰이 있는지 (-qa 가 -qastages 에 걸리지 않게 한다)
	bool HasArg(const wchar_t* token)
	{
		const size_t length = ::wcslen(token);
		for (const wchar_t* at = ::wcsstr(::GetCommandLineW(), token); at; at = ::wcsstr(at + 1, token))
		{
			const wchar_t next = at[length];
			if (next == L'\0' || next == L' ' || next == L'\t')
				return true;
		}
		return false;
	}

	float ArgFloat(const wchar_t* prefix, float fallback)
	{
		const wchar_t* at = ::wcsstr(::GetCommandLineW(), prefix);
		return at ? static_cast<float>(::_wtof(at + ::wcslen(prefix))) : fallback;
	}

	const char* SceneName(SceneId id)
	{
		switch (id)
		{
		case SceneId::MainMenu:   return "MainMenu";
		case SceneId::Lobby:      return "Lobby";
		case SceneId::Loading:    return "Loading";
		case SceneId::FirstGame:  return "FirstGame";
		case SceneId::SecondGame: return "SecondGame";
		case SceneId::ThirdGame:  return "ThirdGame";
		case SceneId::FourthGame: return "FourthGame";
		case SceneId::VGame:      return "VGame";
		case SceneId::LGame:      return "LGame";
		case SceneId::Plaza:      return "Plaza";
		default:                  return "None";
		}
	}

	const char* PhaseName(uint8 phase)
	{
		switch (static_cast<WavePhaseType>(phase))
		{
		case WavePhaseType::Prepare:  return "Prepare";
		case WavePhaseType::Conquest: return "Conquest";
		case WavePhaseType::Escort:   return "Escort";
		case WavePhaseType::Boss:     return "Boss";
		case WavePhaseType::Fail:     return "Fail";
		case WavePhaseType::Clear:    return "Clear";
		default:                      return "None";
		}
	}

	template<typename T>
	T* First(World* world)
	{
		if (!world->HasComponentPool<T>())
			return nullptr;
		auto entities = world->GetEntitiesWithComponent<T>();
		return entities.empty() ? nullptr : world->GetComponent<T>(entities[0]);
	}

	template<typename T>
	T* Singleton(World* world)
	{
		return world->HasComponentPool<T>() ? world->GetSingleton<T>() : nullptr;
	}

	Entity LocalPlayer(World* world)
	{
		if (!world->HasComponentPool<LocalPlayerComponent>() || !world->HasComponentPool<TransformComponent>())
			return Entity();
		auto players = world->GetEntitiesWithComponents<TransformComponent, LocalPlayerComponent>();
		return players.empty() ? Entity() : players[0];
	}

	float HorizontalDistance(const Vec3& a, const Vec3& b)
	{
		const float dx = a.x - b.x;
		const float dz = a.z - b.z;
		return std::sqrt(dx * dx + dz * dz);
	}

	bool IsFinite(const Vec3& v)
	{
		return std::isfinite(v.x) && std::isfinite(v.y) && std::isfinite(v.z);
	}
}

bool QaBot::Requested()
{
	static const bool sRequested = HasArg(L"-qa");
	return sRequested;
}

QaBot& QaBot::Get()
{
	static QaBot sInstance;
	return sInstance;
}

QaBot::QaBot()
{
	mLog = ::_fsopen("qa_log.txt", "w", _SH_DENYNO);
	mStageLimit = std::clamp(
		static_cast<int>(ArgFloat(L"-qastages=", static_cast<float>(kStageCount))), 0, static_cast<int>(kStageCount));
	mFirstStage = mStageIndex = std::clamp(static_cast<int>(ArgFloat(L"-qafirst=", 1.f)) - 1, 0, static_cast<int>(kStageCount) - 1);
	mPhaseTimeout = ArgFloat(L"-qaphasetimeout=", 300.f);
	mRunTimeout = ArgFloat(L"-qatimeout=", 1800.f);
	mCharacter = std::clamp(static_cast<int>(ArgFloat(L"-qachar=", 0.f)), 0, 3);
	Info("START stages=%d phaseTimeout=%.0f runTimeout=%.0f character=%d", mStageLimit, mPhaseTimeout, mRunTimeout, mCharacter);
}

void QaBot::Write(const char* level, const char* format, va_list args)
{
	if (mLog == nullptr)
		return;

	char message[512]{};
	::vsnprintf(message, sizeof(message), format, args);
	::fprintf(mLog, "[QA] t=%.1f scene=%s phase=%s %s %s\n",
		mTime, SceneName(mScene), PhaseName(mPhase), level, message);
	::fflush(mLog);
}

void QaBot::Info(const char* format, ...)
{
	va_list args;
	va_start(args, format);
	Write("INFO", format, args);
	va_end(args);
}

void QaBot::Warn(const char* key, const char* format, ...)
{
	if (++mWarnCounts[key] > kMaxLogsPerKey)
		return;

	va_list args;
	va_start(args, format);
	Write((std::string("WARN ") + key).c_str(), format, args);
	va_end(args);
}

void QaBot::Fail(const char* key, const char* format, ...)
{
	if (++mFailCounts[key] > kMaxLogsPerKey)
		return;

	va_list args;
	va_start(args, format);
	Write((std::string("FAIL ") + key).c_str(), format, args);
	va_end(args);
}

void QaBot::Finish()
{
	if (mFinished)
		return;
	mFinished = true;

	int fails = 0;
	int warns = 0;
	for (const auto& [key, count] : mFailCounts)
	{
		Info("SUMMARY FAIL %s x%d", key.c_str(), count);
		fails += count;
	}
	for (const auto& [key, count] : mWarnCounts)
	{
		Info("SUMMARY WARN %s x%d", key.c_str(), count);
		warns += count;
	}

	Info("RHYTHM judgements perfect=%d good=%d miss=%d crits=%d rhythmChanges=%d skills=%d ultimates=%d",
		mJudgements[2], mJudgements[1], mJudgements[0], mCrits, mRhythmChanges, mSkillsPressed, mUltimates);

	const bool passed = (fails == 0 && mStagesCleared >= mStageLimit - mFirstStage);
	Info("END result=%s stages=%d/%d fails=%d warns=%d deaths=%d targetsDown=%d",
		passed ? "PASS" : "FAIL", mStagesCleared, mStageLimit - mFirstStage, fails, warns, mDeaths, mTargetsDown);
	::PostQuitMessage(0);
}

void QaBot::LoadNav(SceneId sceneId)
{
	if (mNavQuery)
	{
		dtFreeNavMeshQuery(mNavQuery);
		mNavQuery = nullptr;
	}
	mNavMesh.reset();

	const wchar_t* file = NavFile(sceneId);
	if (file == nullptr)
		return;

	// ponytail: 서버 리소스 폴더를 직접 읽는다 (클라이언트에는 스테이지별 NavMesh 가 없다).
	//           배포본에서 QA 를 돌리려면 클라이언트 Resources 로 복사하고 경로를 바꾼다.
	auto navMesh = std::make_shared<NavMesh>();
	navMesh->Load(std::wstring(L"..\\..\\Server\\Resources\\NavMesh\\") + file);
	dtNavMeshQuery* query = navMesh->mDtNavMesh ? dtAllocNavMeshQuery() : nullptr;
	if (query == nullptr || dtStatusFailed(query->init(navMesh->mDtNavMesh, 2048)))
	{
		if (query)
			dtFreeNavMeshQuery(query);
		Warn("navmesh", "%ls 를 열지 못함, 직선 주행으로 진행", file);
		return;
	}
	mNavMesh = navMesh;
	mNavQuery = query;
}

Vec3 QaBot::PathStep(const Vec3& position, const Vec3& target)
{
	if (mNavQuery == nullptr)
		return target;

	// 엔진(cm) -> NavMesh(m), 축 교환 포함 (서버 NavMeshLoader 와 같은 규칙)
	const float start[3] = { position.z * 0.01f, position.y * 0.01f, position.x * 0.01f };
	const float end[3] = { target.z * 0.01f, target.y * 0.01f, target.x * 0.01f };
	const float extents[3] = { 5.f, 10.f, 5.f };
	const dtQueryFilter filter;

	dtPolyRef startRef = 0, endRef = 0;
	float startOnMesh[3], endOnMesh[3];
	mNavQuery->findNearestPoly(start, extents, &filter, &startRef, startOnMesh);
	mNavQuery->findNearestPoly(end, extents, &filter, &endRef, endOnMesh);
	if (!startRef || !endRef)
		return target;

	dtPolyRef polys[256];
	int polyCount = 0;
	if (dtStatusFailed(mNavQuery->findPath(startRef, endRef, startOnMesh, endOnMesh, &filter, polys, &polyCount, 256))
		|| polyCount == 0)
		return target;

	float corners[8 * 3];
	int cornerCount = 0;
	mNavQuery->findStraightPath(startOnMesh, endOnMesh, polys, polyCount, corners, nullptr, nullptr, &cornerCount, 8);
	for (int i = 1; i < cornerCount; ++i)	// 0 번은 출발점
	{
		const Vec3 corner(corners[i * 3 + 2] * 100.f, corners[i * 3 + 1] * 100.f, corners[i * 3] * 100.f);
		if (HorizontalDistance(position, corner) > kWaypointReachDistance)
			return corner;
	}
	return target;
}

void QaBot::AbandonStage(World* world)
{
	Info("STAGE abandon %s after %.1fs", SceneName(mScene), mSceneTime);
	if (++mStageIndex >= mStageLimit)
	{
		Finish();
		return;
	}
	// 레벨 -> 다른 레벨 요청은 서버의 디버그 강제 전환 경로를 탄다 (SceneManager::TryChangeScene).
	RequestScene(world, kStageOrder[mStageIndex], false);
}

void QaBot::OnSceneChanged(SceneId sceneId)
{
	// 레벨을 클리어하고 나왔으면 다음 스테이지로 넘어간다.
	if (IsLevelScene(mScene) && mStageCleared)
	{
		++mStagesCleared;
		++mStageIndex;
	}

	const SceneId previous = mScene;
	mScene = sceneId;
	mPhase = 0;
	Info("SCENE %s -> %s after %.1fs", SceneName(previous), SceneName(sceneId), mSceneTime);

	if (sceneId == mRequestedScene)
		mRequestedScene = SceneId::End;

	mSceneTime = 0.f;
	mRetryTimer = 0.f;
	mPhaseTime = 0.f;
	mStageCleared = false;
	mMoveTestDone = false;
	mMoveTestStarted = false;
	mHasSpawnHeight = false;
	mWasDead = false;
	mStuckWindow = 0.f;
	mStuckTime = 0.f;
	mUnstickTimer = 0.f;
	mAttackTarget = Entity();
	mAttackTime = 0.f;
	mPathTimer = 0.f;
	mStageStuckTime = 0.f;
	mRhythmChangeTimer = 0.f;
	mRhythmCheckTimer = -1.f;
	mActionWasDown = false;
	LoadNav(sceneId);
}

void QaBot::RequestScene(World* world, SceneId target, bool retry)
{
	if (mRequestedScene != target)
	{
		mRequestedScene = target;
		mRequestElapsed = 0.f;
		Info("REQUEST scene %s", SceneName(target));
		world->GetEventManager()->Enqueue(EvNetSceneChange{ target });
		return;
	}

	if (retry)
		world->GetEventManager()->Enqueue(EvNetSceneChange{ target });	// 서버가 조건을 다시 검사한다
}

void QaBot::ProduceInput(InputFrame& frame)
{
	if (mFinished)
		return;

	const float dt = gEngine->GetTimer().GetTimeElapsed();
	mTime += dt;

	auto scene = gEngine->GetSceneManager().GetActiveScene();
	if (!scene || !scene->GetWorld())
		return;

	World* world = scene->GetWorld().get();
	const SceneId sceneId = scene->GetSceneId();
	if (sceneId != mScene)
		OnSceneChanged(sceneId);

	mSceneTime += dt;
	mRetryTimer += dt;
	const bool retry = (mRetryTimer >= kRetrySec);
	if (retry)
		mRetryTimer = 0.f;

	if (mTime > mRunTimeout)
	{
		Fail("run-timeout", "전체 실행 제한 %.0fs 초과", mRunTimeout);
		Finish();
		return;
	}

	if (mRequestedScene != SceneId::End)
	{
		const float before = mRequestElapsed;
		mRequestElapsed += dt;
		if (before < kSceneChangeTimeoutSec && mRequestElapsed >= kSceneChangeTimeoutSec)
			Fail("scene-change-timeout", "%s 요청 후 %.0fs 동안 전환되지 않음",
				SceneName(mRequestedScene), kSceneChangeTimeoutSec);
	}

	if (IsRoomScene(sceneId) && mSceneTime > kSettleSec && dt > kHitchSec)
		Warn("hitch", "프레임 %.0fms", dt * 1000.f);

	switch (sceneId)
	{
	case SceneId::MainMenu:
		TickMainMenu(world, retry);
		break;
	case SceneId::Lobby:
		TickLobby(world, retry);
		break;
	case SceneId::Plaza:
		TickPlaza(world, frame, dt, retry);
		break;
	case SceneId::FirstGame:
	case SceneId::SecondGame:
	case SceneId::ThirdGame:
	case SceneId::FourthGame:
		TickLevel(world, frame, dt, retry);
		break;
	case SceneId::VGame:
	case SceneId::LGame:
		if (mSceneTime > kSettleSec)
			Finish();
		break;
	default:
		break;
	}
}

void QaBot::TickMainMenu(World* world, bool retry)
{
	// GAME START 버튼과 같은 동작: RoomList 상태 전환 + 서버 접속.
	// 방에 들어가면 MenuRoomBrowserSystem 이 Lobby 로 전환한다.
	auto* menu = First<MainMenuController>(world);
	if (!menu || !retry)
		return;

	if (menu->mState != MainMenuState::RoomList)
	{
		menu->Request(MainMenuState::RoomList);
		return;
	}
	if (!Network::GetInstance().Awake())
	{
		Warn("connect", "서버 접속 실패, 재시도");
		return;
	}

	auto* list = First<LobbyRoomListComponent>(world);
	if (!list || list->mCurrentRoomId == 0)
		world->GetEventManager()->Enqueue(EvRoomCreate{});	// CREATE ROOM 버튼과 같은 이벤트
}

void QaBot::TickLobby(World* world, bool retry)
{
	auto* state = First<LobbyRoomStateComponent>(world);
	if (!retry || !state || !state->mHasSnapshot)
		return;

	const uint32 me = Network::GetInstance().mClientId;
	bool meReady = false;
	uint8 myType = 0;
	for (const auto& slot : state->mSlots)
		if (slot.sessionId != 0 && slot.sessionId == me)
		{
			meReady = slot.ready;
			myType = slot.playerType;
		}

	// 캐릭터를 바꾸면 서버가 ready 를 풀기 때문에 캐릭터부터 맞춘다 (좌우 화살표가 발행하는 것과 같은 이벤트).
	if (mCharacter > 0 && myType != mCharacter - 1)
	{
		if (auto* choice = First<ChoicePlayerComponent>(world))
			choice->mPlayerType = static_cast<uint8>(mCharacter - 1);
		world->GetEventManager()->Enqueue(EvRoomCharacterChanged{ static_cast<uint8>(mCharacter - 1) });
	}
	else if (!meReady)
		world->GetEventManager()->Enqueue(EvRoomReadyChanged{ true });
	else
		RequestScene(world, SceneId::Plaza, true);
}

void QaBot::TickPlaza(World* world, InputFrame& frame, float dt, bool retry)
{
	const Entity player = LocalPlayer(world);
	if (!player.IsValid() || mSceneTime < 3.f || Cinematic::IsAnyCinematicPlaying(world))
		return;

	auto* transform = world->GetComponent<TransformComponent>(player);
	const Vec3 position = transform->mWorldPosition;

	// 이동 점검: 전진 키를 누르고 실제로 움직이는지 본다.
	if (!mMoveTestDone)
	{
		if (!mMoveTestStarted)
		{
			mMoveTestStarted = true;
			mMoveTestStart = position;
			mMoveTestTime = 0.f;
		}
		mMoveTestTime += dt;
		frame.keys[static_cast<size_t>(eKeyCode::W)] = true;

		const float moved = HorizontalDistance(position, mMoveTestStart);
		if (moved >= kMoveTestDistance)
		{
			mMoveTestDone = true;
			Info("MOVE ok %.0fcm in %.1fs", moved, mMoveTestTime);
		}
		else if (mMoveTestTime >= kMoveTestTimeoutSec)
		{
			mMoveTestDone = true;
			Fail("move", "전진 입력 %.0fs 동안 %.0fcm 이동 pos=(%.0f,%.0f,%.0f)",
				kMoveTestTimeoutSec, moved, position.x, position.y, position.z);
		}
		return;
	}

	if (mStageIndex >= mStageLimit)
	{
		Finish();
		return;
	}
	RequestScene(world, kStageOrder[mStageIndex], retry);
}

void QaBot::TickLevel(World* world, InputFrame& frame, float dt, bool retry)
{
	// 포기하고 다른 레벨을 요청해 둔 상태면 전환만 기다린다.
	if (IsLevelScene(mRequestedScene))
	{
		RequestScene(world, mRequestedScene, retry);
		return;
	}

	// ---- 박자 시계와 지난 프레임의 판정 결과 ----
	BeatSystem* beat = world->GetSystemManager() ? world->GetSystemManager()->GetSystem<BeatSystem>() : nullptr;
	const float songPos = beat ? beat->GetSongPosition() : 0.f;
	const float beatSec = beat ? beat->GetBeatSeconds() : 0.f;
	if (songPos != mLastSongPos)	// 월드가 한 프레임 돌았을 때만 센다 (이벤트 버퍼는 다음 스왑까지 남아 있다)
	{
		mLastSongPos = songPos;
		world->GetEventManager()->Consume<EvBeatJudgement>([this](const EvBeatJudgement& e) {
			if (!e.predicted && e.judgement < 3)
				++mJudgements[e.judgement];
		});
		world->GetEventManager()->Consume<EvHealthChanged>([this, world](const EvHealthChanged& e) {
			if (e.isCritical && e.hp < e.previousHp && world->HasComponentPool<EnemyComponent>()
				&& world->GetComponent<EnemyComponent>(e.target))
				++mCrits;
		});
	}

	// ---- 페이즈 진행 ----
	const GameRuleComponent* rule = Singleton<GameRuleComponent>(world);
	const uint8 phase = rule ? rule->mGamePhase : 0;
	if (phase != mPhase)
	{
		Info("PHASE %s -> %s after %.1fs", PhaseName(mPhase), PhaseName(phase), mPhaseTime);
		mPhase = phase;
		mPhaseTime = 0.f;
	}
	mPhaseTime += dt;

	const WavePhaseType phaseType = static_cast<WavePhaseType>(phase);
	if (phaseType == WavePhaseType::Clear)
	{
		if (!mStageCleared)
		{
			mStageCleared = true;
			Info("STAGE clear in %.1fs", mSceneTime);
		}
		if (mPhaseTime > 3.f)
			RequestScene(world, SceneId::Plaza, retry);	// 결과판 LEAVE 버튼과 같은 이벤트
		return;
	}
	if (phaseType == WavePhaseType::Fail)
	{
		Fail("stage-failed", "스테이지 실패 (Fail 페이즈)");
		AbandonStage(world);
		return;
	}
	if (mPhaseTime > mPhaseTimeout)
	{
		Fail("phase-timeout", "%s 페이즈가 %.0fs 동안 끝나지 않음", PhaseName(phase), mPhaseTimeout);
		AbandonStage(world);
		return;
	}
	if (mStageStuckTime >= kStuckAbandonSec)
	{
		Fail("stage-abandoned-stuck", "끼임 누적 %.0fs, 스테이지 포기", mStageStuckTime);
		AbandonStage(world);
		return;
	}

	// ---- 플레이어 관찰 ----
	const Entity player = LocalPlayer(world);
	if (!player.IsValid())
		return;

	auto* transform = world->GetComponent<TransformComponent>(player);
	auto* mainPlayer = world->GetComponent<MainPlayerComponent>(player);
	const Vec3 position = transform->mWorldPosition;

	if (!IsFinite(position))
	{
		Fail("position-nan", "플레이어 위치가 유한하지 않음");
		return;
	}
	if (!mHasSpawnHeight)
	{
		mHasSpawnHeight = true;
		mSpawnHeight = position.y;
	}
	else if (position.y < mSpawnHeight - kFallDistance)
	{
		Fail("fell-out", "맵 밖 낙하 pos=(%.0f,%.0f,%.0f) spawnY=%.0f",
			position.x, position.y, position.z, mSpawnHeight);
	}

	const HealthComponent* playerHealth =
		world->HasComponentPool<HealthComponent>() ? world->GetComponent<HealthComponent>(player) : nullptr;
	const bool dead = (mainPlayer && (mainPlayer->mFlags & FLAG_DEAD)) || (playerHealth && playerHealth->IsDead());
	if (dead && !mWasDead)
		Info("DEATH #%d pos=(%.0f,%.0f,%.0f)", ++mDeaths, position.x, position.y, position.z);
	mWasDead = dead;
	if (dead || Cinematic::IsAnyCinematicPlaying(world))
	{
		mStuckWindow = 0.f;	// 조작할 수 없는 동안은 끼임으로 세지 않는다
		mRhythmCheckTimer = -1.f;
		return;
	}

	// ---- 목표 지점 ----
	bool hasObjective = false;
	Vec3 objective{};
	if (phaseType == WavePhaseType::Conquest)
	{
		if (const GameConquestComponent* conquest = Singleton<GameConquestComponent>(world);
			conquest && conquest->mActiveZoneId != 0)
		{
			hasObjective = true;
			objective = conquest->mActiveZonePos;
		}
	}
	else if (phaseType == WavePhaseType::Escort)
	{
		if (const GameEscortComponent* escort = Singleton<GameEscortComponent>(world);
			escort && escort->mEscortTarget.IsValid())
		{
			if (auto* truck = world->GetComponent<TransformComponent>(escort->mEscortTarget))
			{
				hasObjective = true;
				objective = truck->mWorldPosition;
			}
		}
	}

	// ---- 가장 가까운 살아 있는 적 ----
	// 호위 중에는 트럭 주변의 적만 대상으로 삼는다. 서버는 트럭 반경 안에 적이 있으면 트럭을 세운다.
	const bool escorting = hasObjective && phaseType == WavePhaseType::Escort;
	Entity enemy;
	Vec3 enemyPosition{};
	float enemyDistance = FLT_MAX;
	int enemiesAlive = 0;
	const HealthComponent* enemyHealth = nullptr;
	const EnemyComponent* enemyInfo = nullptr;
	if (world->HasComponentPool<EnemyComponent>())
	{
		for (Entity e : world->GetEntitiesWithComponents<TransformComponent, EnemyComponent>())
		{
			const EnemyComponent* info = world->GetComponent<EnemyComponent>(e);
			const HealthComponent* health =
				world->HasComponentPool<HealthComponent>() ? world->GetComponent<HealthComponent>(e) : nullptr;
			// Obelisk 는 부서져도 곧 되살아나는 고정 석상이다. 대상으로 삼으면 옆에 붙어 끝없이 부수기만 한다.
			if (info->mEnemyType == EnemyType::Obelisk)
				continue;
			if (info->mAnimState == static_cast<int>(EnemyAnimState::Dead) || (health && health->IsDead()))
				continue;

			const Vec3 at = world->GetComponent<TransformComponent>(e)->mWorldPosition;
			const float distance = HorizontalDistance(position, at);
			++enemiesAlive;
			if (escorting && HorizontalDistance(at, objective) > kObjectiveLeash)
				continue;
			if (distance < enemyDistance)
			{
				enemy = e;
				enemyPosition = at;
				enemyDistance = distance;
				enemyHealth = health;
				enemyInfo = info;
			}
		}
	}

	// 공격하던 대상이 사라졌거나 죽었으면 처치로 센다.
	if (mAttackTarget.IsValid() && !(enemy == mAttackTarget))
	{
		const HealthComponent* previous =
			world->HasComponentPool<HealthComponent>() ? world->GetComponent<HealthComponent>(mAttackTarget) : nullptr;
		if (previous == nullptr || previous->IsDead())
			++mTargetsDown;
		mAttackTarget = Entity();
		mAttackTime = 0.f;
	}

	// ---- 행동 결정 ----
	// 점령: 구역을 지키고 다가오는 적을 제자리에서 공격한다 (구역을 벗어나면 점령이 안 오른다).
	// 호위: 트럭을 막는 적이 있으면 그 적에게 붙고, 없으면 트럭 옆으로 간다.
	// 그 외: 가장 가까운 적에게 붙는다.
	// 점령 중이라도 바로 옆에서 길을 막는 적은 먼저 상대한다.
	const bool holdingZone = hasObjective && phaseType == WavePhaseType::Conquest;
	const bool chaseEnemy = enemy.IsValid() && (!holdingZone || enemyDistance < kBlockerDistance);
	const bool towardEnemy = chaseEnemy && enemyDistance > kMeleeDistance;
	const bool towardObjective = hasObjective && !chaseEnemy &&
		HorizontalDistance(position, objective) > kObjectiveHoldDistance;

	if (towardObjective)
		Steer(world, frame, dt, position, objective, true);
	else if (enemy.IsValid() && (towardEnemy || enemyDistance < kEngageDistance))
		Steer(world, frame, dt, position, enemyPosition, towardEnemy);
	else
		mStuckWindow = 0.f;

	// ---- 진행 상황 기록 (실패했을 때 무엇을 하고 있었는지 남긴다) ----
	mStatusTimer += dt;
	if (mStatusTimer >= kStatusIntervalSec)
	{
		mStatusTimer = 0.f;
		const GameConquestComponent* conquest = Singleton<GameConquestComponent>(world);
		const GameEscortComponent* escort = Singleton<GameEscortComponent>(world);
		Info("STATUS pos=(%.0f,%.0f,%.0f) objectiveDist=%.0f enemies=%d nearest=%.0f conquest=%.1f/%.1f escort=%.2f stage=%d/%d truckMoving=%d",
			position.x, position.y, position.z,
			hasObjective ? HorizontalDistance(position, objective) : -1.f,
			enemiesAlive, enemy.IsValid() ? enemyDistance : -1.f,
			conquest ? conquest->mWaveTime : 0.f, conquest ? conquest->mRequiredConquestTime : 0.f,
			escort ? escort->mEscortProgress : 0.f, escort ? escort->mEscortStage : 0, escort ? escort->mStageCount : 0,
			escort ? escort->mMoveState : 0);
	}

	// ---- 공격 ----
	auto* movement = world->GetComponent<PlayerMovementComponent>(player);
	bool attacking = false;
	if (enemy.IsValid() && enemyDistance < kEngageDistance && movement)
	{
		const float yawToEnemy = MathUtils::YawDegreesFromDir(enemyPosition - position);
		const float aimError = std::fabs(MathUtils::Wrap180Degrees(yawToEnemy - movement->mCameraRotationY));
		attacking = (aimError < kAimToleranceDeg);
	}

	// 박자에 맞춰 누른다. 서버는 버튼이 눌린 순간의 곡 위치로 판정하고 Perfect 면 치명타다.
	// 박자 직전(kBeatLeadSec)에 눌러 잠깐 유지했다 떼므로 박자마다 눌림 가장자리가 한 번 생긴다.
	float beatOffset = beatSec > 0.f ? std::fmod(songPos, beatSec) : 0.f;	// 가장 가까운 박자까지의 거리 (음수 = 박자 직전)
	if (beatOffset > beatSec * 0.5f)
		beatOffset -= beatSec;
	const bool onBeat = beatSec > 0.f && beatOffset > -kBeatLeadSec && beatOffset < kBeatHoldSec;
	const int64 beatIndex = beatSec > 0.f ? std::llround(songPos / beatSec) : 0;

	// 궁극기: 리듬 포인트가 가득 차면 Q
	const int rhythmPoints = mainPlayer ? mainPlayer->mRhythmPoints : 0;
	if (mLastRhythmPoints >= MainPlayerComponent::kMaxRhythmPoints && rhythmPoints < mLastRhythmPoints)
		Info("ULTIMATE #%d used", ++mUltimates);
	mLastRhythmPoints = rhythmPoints;
	mUltimateHold -= dt;

	if (attacking && onBeat)
	{
		eKeyCode skill = eKeyCode::End;
		if (rhythmPoints >= MainPlayerComponent::kMaxRhythmPoints && mUltimateHold <= 0.f)
		{
			skill = eKeyCode::Q;
			mUltimateHold = kUltimateHoldSec;
		}
		else if (beatIndex % kSkill2EveryBeats == kSkill2EveryBeats - 1)
			skill = eKeyCode::SHIFT;
		else if (beatIndex % kSkill1EveryBeats == kSkill1EveryBeats / 2)
			skill = eKeyCode::E;

		if (skill == eKeyCode::End)
			frame.mouseLeft = true;
		else
		{
			frame.keys[static_cast<size_t>(skill)] = true;
			if (!mActionWasDown && skill != eKeyCode::Q)
				++mSkillsPressed;
		}
	}
	mActionWasDown = attacking && onBeat;

	if (attacking)
	{
		// 붙어서 계속 때리는데 체력이 줄지 않으면 판정이 안 들어가는 것이다.
		// Slime 은 치명타만 피해를 받지만 (서버 DamageSystem) 박자를 맞추므로 여기서 함께 검증된다.
		if (enemyHealth && enemyDistance < kMeleeDistance + 80.f)
		{
			if (!(enemy == mAttackTarget))
			{
				mAttackTarget = enemy;
				mAttackTargetHp = enemyHealth->mCurrentHp;
				mAttackTime = 0.f;
			}
			else if (enemyHealth->mCurrentHp < mAttackTargetHp)
			{
				mAttackTargetHp = enemyHealth->mCurrentHp;
				mAttackTime = 0.f;
			}
			else if (mUltimateHold <= kUltimateHoldSec - kUltimateBusySec)	// 궁극기 중에는 기본 공격이 나가지 않는다
			{
				mAttackTime += dt;
				if (mAttackTime >= kAttackIneffectiveSec)
				{
					Fail("attack-ineffective", "%.0fs 동안 근접 공격했지만 체력 변화 없음 enemyType=%d animState=%d hp=%d dist=%.0f",
						kAttackIneffectiveSec, static_cast<int>(enemyInfo->mEnemyType), enemyInfo->mAnimState,
						enemyHealth->mCurrentHp, enemyDistance);
					mAttackTime = 0.f;
				}
			}
		}
	}

	if (mainPlayer && mainPlayer->mMaxBullet > 0 && mainPlayer->mNowBullet <= 0)
		frame.keys[static_cast<size_t>(eKeyCode::R)] = true;

	// ---- 리듬 전환 (우클릭) ----
	// 일정 간격으로 다음 리듬을 요청하고, 서버가 예약한 루프 경계가 지나 실제로 바뀌었는지 확인한다.
	if (mainPlayer == nullptr)
		return;
	if (mRhythmCheckTimer >= 0.f)
	{
		mRhythmCheckTimer += dt;
		if (mRhythmClickPending)
		{
			// 클라이언트가 클릭을 받았으면 다음 프레임에 mDesiredRhythm 이 바뀌어 있다.
			// 안 바뀌었으면 입력 단계에서 무시된 것(공중·착지, 컷신 등)이라 실패가 아니라 재시도 대상이다.
			mRhythmClickPending = false;
			if (mainPlayer->mDesiredRhythm != mRhythmExpected)
			{
				Info("RHYTHM click ignored lowerState=%d state=%d", mainPlayer->mLowerState, static_cast<int>(mainPlayer->GetState()));
				mRhythmCheckTimer = -1.f;
				mRhythmChangeTimer = kRhythmChangeIntervalSec - 1.f;
				return;
			}
		}
		if (mainPlayer->mRhythm == mRhythmExpected)
		{
			Info("RHYTHM #%d -> %d in %.2fs", ++mRhythmChanges, mRhythmExpected, mRhythmCheckTimer);
			mRhythmCheckTimer = -1.f;
		}
		else if (mRhythmCheckTimer >= kRhythmChangeCheckSec)
		{
			Fail("rhythm-change", "전환 요청 후 %.1fs 가 지나도 리듬이 %d 로 바뀌지 않음 (현재 %d, 예약 %d, desired=%d inFlight=%d settle=%.2f lowerState=%d state=%d)",
				kRhythmChangeCheckSec, mRhythmExpected, mainPlayer->mRhythm, mainPlayer->mHasQueuedRhythmChange ? 1 : 0,
				mainPlayer->mDesiredRhythm, mainPlayer->mRhythmChangeInFlight ? 1 : 0, mainPlayer->mRhythmSettleTimer,
				mainPlayer->mLowerState, static_cast<int>(mainPlayer->GetState()));
			mRhythmCheckTimer = -1.f;
		}
		return;
	}

	mRhythmChangeTimer += dt;
	const bool grounded = (mainPlayer->mFlags & FLAG_JUMP) == 0 && mUnstickTimer <= 0.f;	// 공중에서는 전환 입력이 무시된다
	if (mRhythmChangeTimer >= kRhythmChangeIntervalSec && grounded && phaseType != WavePhaseType::Prepare)
	{
		mRhythmChangeTimer = 0.f;
		mRhythmExpected = ToRhythmValue(GetNextRhythm(SanitizeRhythm(mainPlayer->mDesiredRhythm)));
		mRhythmCheckTimer = 0.f;
		mRhythmClickPending = true;
		frame.mouseRight = true;
	}
}

void QaBot::Steer(World* world, InputFrame& frame, float dt, const Vec3& position, const Vec3& target, bool move)
{
	const Entity player = LocalPlayer(world);
	auto* movement = world->GetComponent<PlayerMovementComponent>(player);
	if (movement == nullptr)
		return;

	// 이동할 때는 NavMesh 경로의 다음 지점을 향한다 (제자리 조준은 대상을 직접 본다).
	Vec3 aim = target;
	if (move)
	{
		mPathTimer -= dt;
		if (mPathTimer <= 0.f)
		{
			mPathTimer = kPathReplanSec;
			mPathNext = PathStep(position, target);
		}
		aim = mPathNext;
	}
	else
	{
		mPathTimer = 0.f;
	}

	// 카메라 yaw 를 목표 방향으로, pitch 는 수평으로 맞춘다. 마우스 1픽셀이 0.5도라 오차가 매 프레임 절반씩 준다.
	const float targetYaw = MathUtils::YawDegreesFromDir(aim - position);
	const float yawError = MathUtils::Wrap180Degrees(targetYaw - movement->mTargetRotation.y);
	frame.mouseDelta.x = std::clamp(std::lround(yawError), -kMaxMousePixels, kMaxMousePixels);
	frame.mouseDelta.y = std::clamp(std::lround(-movement->mTargetRotation.x), -kMaxMousePixels, kMaxMousePixels);

	if (!move)
	{
		mStuckWindow = 0.f;
		return;
	}

	if (std::fabs(yawError) < kMoveToleranceDeg)
		frame.keys[static_cast<size_t>(eKeyCode::W)] = true;

	// 경로를 따라가도 막히면 점프·옆걸음으로 빠져나온다.
	if (mUnstickTimer > 0.f)
	{
		mUnstickTimer -= dt;
		frame.keys[static_cast<size_t>(eKeyCode::SPACE)] = true;
		frame.keys[static_cast<size_t>(mUnstickLeft ? eKeyCode::A : eKeyCode::D)] = true;
	}

	if (mStuckWindow <= 0.f)
		mStuckWindowStart = position;
	mStuckWindow += dt;
	if (mStuckWindow < kStuckWindowSec)
		return;

	const float moved = HorizontalDistance(position, mStuckWindowStart);
	mStuckWindow = 0.f;
	if (moved >= kStuckDistance)
	{
		mStuckTime = 0.f;
		return;
	}

	mStuckTime += kStuckWindowSec;
	mStageStuckTime += kStuckWindowSec;
	mUnstickTimer = kUnstickSec;
	mUnstickLeft = !mUnstickLeft;
	if (mStuckTime >= kStuckFailSec)
	{
		Fail("stuck", "%.0fs 동안 전진 불가 pos=(%.0f,%.0f,%.0f) target=(%.0f,%.0f,%.0f)",
			mStuckTime, position.x, position.y, position.z, target.x, target.y, target.z);
		mStuckTime = 0.f;
	}
	else
	{
		Warn("stuck", "%.0fs 동안 %.0fcm 이동 pos=(%.0f,%.0f,%.0f)",
			kStuckWindowSec, moved, position.x, position.y, position.z);
	}
}
