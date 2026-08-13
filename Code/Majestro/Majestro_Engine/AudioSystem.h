#pragma once
#include "System.h"
#include "World.h"
#include "ComponentPool.h"
#include "AudioManager.h"
#include "Protocol/RhythmDefinitions.h"

enum PlayerType : uint8;

// 보스 타입별 전용 음악 설정
struct BossMusicConfig
{
	uint8 enemyType;        // EnemyType
	SOUNDNAME stem;         // 전용 BGM 슬롯
	const char* eventPath;  // FMOD 이벤트 경로
	const char* paramName;  // 스킬 연출 파라미터 (0 = 기본 테마, 1~4 = 스킬 번호)
};

// 전용 음악을 가진 보스 수. 설정 테이블과 런타임 상태 배열 크기를 함께 고정한다.
constexpr size_t kBossMusicSlotCount = 2;

class AudioSystem : public System
{
public:
	AudioSystem(World* world);

	void Initialize();
	void Update(float);
	void Shutdown();
private:
	float time{};
	int mPrevEscortBgmStage = -1;   // 호위 진행도 BGM 단계(0~2) 변화 감지용 (-1 = 미초기화)

	bool mBgmStartAligned = false;	// 시킹 완료 후 재생까지 대기 중인지
	int  mAlignSeekFrame = -1;   // -1=시킹 전, 0~=시킹 후 경과 프레임
	void AlignBgmToServerSongClock();
	bool mBgmInitializationFailed = false; // 세 리듬 스템 중 하나라도 준비하지 못했는지

	// 드리프트 보정(피치 너지): FMOD 재생 위상이 박자 위상에서 벌어지면 재생 속도를 미세 조정해 끊김 없이 수렴.
	void CorrectBgmDrift();
	const float mLoopLen = kMusicLoopSeconds; // 128 BPM에서 16박자 루프는 7.5초다.
	float mDriftLogTimer = 0.f;    // 드리프트 로그 저빈도 출력용 누적기
	float mDriftSmoothed = 0.f;    // EMA 평활화된 drift(초) — 측정 노이즈 제거 후 이 값으로 보정
	float mDriftEmaAlpha = 0.15f;  // EMA 계수(작을수록 더 부드럽고 느림)
	float mDriftDeadzone = 0.012f; // 이내는 보정 안 함
	float mDriftMaxNudge = 0.008f; // 최대 속도 변화
	float mDriftGain = 0.1f;       // drift 대비 너지 비례 계수

    void ApplyRhythmToStem(PlayerType playerType, Rhythm rhythm);
	void UpdateSilenceMusicState();


	float WrapToLoop(float seconds) const;

	// 보스 전용 음악
	struct BossMusicState
	{
		bool  playing = false;      // 이 슬롯의 음악을 재생 중인지
		bool  aligned = false;      // 기준 스템 위상 정렬을 마치고 재생 중인지
		int   alignFrame = -1;      // -1 = 시킹 전, 0~ = 시킹 후 경과 프레임
		float resyncCooldown = 0.f; // 하드 시킹 반영 대기 시간(초)
		int   skillIndex = 0;       // 마지막으로 파라미터에 반영한 스킬 번호 (0 = 기본 테마)
	};
	std::array<BossMusicState, kBossMusicSlotCount> mBossMusic{};

	void UpdateBossMusic();
	void StopAllBossMusic();


	void AlignBossMusicToReferenceStem(const BossMusicConfig& config, BossMusicState& state);
	void CorrectBossMusicDrift(float referencePhase, float pitch);

	// 수정사항: Silence 상태가 바뀔 때만 로컬 플레이어 음악의 출력 배율을 변경한다.
	bool mSilenceMusicMuted = false;
	SOUNDNAME mSilenceMusicStem = SOUNDNAME::End;
};

