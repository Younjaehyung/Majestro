#pragma once
#include "System.h"
#include "World.h"
#include "ComponentPool.h"
#include "AudioManager.h"

class AudioSystem : public System
{
public:
	AudioSystem(World* world);

	void Initialize();
	void Update(float);
	void Shutdown();
private:
	float time{};
	int mPrevEscortStage = -1;   // 호위 거점 변화 감지용 (-1 = 미초기화)

	bool mBgmStartAligned = false;	// 시킹 완료 후 재생까지 대기 중인지
	int  mAlignSeekFrame = -1;   // -1=시킹 전, 0~=시킹 후 경과 프레임
	void AlignBgmToServerSongClock();

	// 드리프트 보정(피치 너지): FMOD 재생 위상이 박자 위상에서 벌어지면 재생 속도를 미세 조정해 끊김 없이 수렴.
	void CorrectBgmDrift();
	float mLoopLen = 6.0f;         // BGM 루프 길이(초)
	float mDriftLogTimer = 0.f;    // 드리프트 로그 저빈도 출력용 누적기
	float mDriftSmoothed = 0.f;    // EMA 평활화된 drift(초) — 측정 노이즈 제거 후 이 값으로 보정
	float mDriftEmaAlpha = 0.15f;  // EMA 계수(작을수록 더 부드럽고 느림)
	float mDriftDeadzone = 0.012f; // 이내는 보정 안 함
	float mDriftMaxNudge = 0.008f; // 최대 속도 변화
	float mDriftGain = 0.1f;       // drift 대비 너지 비례 계수

	void ApplyRhythmLayerByPlayerType(uint8 playerType, uint8 rhythm);
	void UpdateSilenceMusicState();

	// 수정사항: Silence 상태가 바뀔 때만 로컬 플레이어 음악의 출력 배율을 변경한다.
	bool mSilenceMusicMuted = false;
	SOUNDNAME mSilenceMusicStem = SOUNDNAME::End;
};

