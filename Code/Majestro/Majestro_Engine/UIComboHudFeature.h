#pragma once
#include "UIFeature.h"
#include "Entity.h"

// 콤보 HUD
class UIComboHudFeature : public UIFeature
{
public:
	void Initialize(World* world) override;
	void Update(float dt) override;

private:
	struct JudgementEntry
	{
		size_t mPoolIndex = 0;      // mFeedPool 슬롯
		uint8 mJudgement = 0;       // 0=Miss, 1=Good, 2=Perfect
		float mAge = 0.0f;          // 생성 후 경과(타임아웃 판정용)
		float mCurrentY = 0.0f;     // 부드러운 스택 이동용 현재 Y
		float mCurrentScale = 1.0f;
		float mCurrentAlpha = 0.0f;
		bool mFadingOut = false;
		float mFadeElapsed = 0.0f;
		float mAlphaAtFadeStart = 0.0f; // 페이드 시작 시점 알파(자연스러운 감쇠 기준)
	};

	void EnsureEntities();
	int ReadLocalCombo() const;

	float ReadBeatBounce() const;
	void UpdateComboCorner(float dt);

	// 랭크 상태 갱신
	void UpdateRankState(int combo, float dt);
	void TriggerRankPop(float scale, float duration);
	void UpdateJudgementFeed(float dt);
	void PushJudgement(uint8 judgement);

	static int RankIndexForCombo(int combo);	// 0=S..4=F

private:
	// 판정 피드 파라미터.
	static constexpr int kFeedMaxVisible = 2;         // 유지되는 최대 판정 수 (최신 + 이전 1개)
	static constexpr float kFeedTimeout = 1.5f;       // 판정별 유지 시간(초)
	static constexpr float kFeedFadeDuration = 0.4f;  // 페이드 아웃 길이
	static constexpr size_t kFeedPoolSize = 6;        // 페이드 중인 항목 포함 동시 표시 상한

	// 랭크 하강 주기
	static constexpr float kRankDecayInterval = 3.0f;
	static constexpr int kRankIndexF = 4; // 기본/최하 랭크

	// 우상단 콤보 표시.
	Entity mRankBackEntity = NULL_ENTITY;  // 랭크 뒤 스플래시 배경(아틀라스 BACK 셀)
	Entity mRankEntity = NULL_ENTITY;
	Entity mCountTextEntity = NULL_ENTITY;
	Entity mComboWordEntity = NULL_ENTITY; // 아틀라스 "COMBO!" 텍스처


	int mLastCombo = 0;
	int mCurrentRankIndex = kRankIndexF;   // 유지되는 현재 랭크 (기본 F)
	int mAppliedRankIndex = -1;            // 소스렉트가 반영된 랭크 (변경 감지용)
	float mRankDecayTimer = 0.0f;          // 콤보 부재 시 하강 누적 시간
	float mCountPopTimer = 0.0f; // 콤보 증가 팝

	// 랭크 팝
	float mRankPopTimer = 0.0f;
	float mRankPopScale = 1.0f;
	float mRankPopDuration = 0.0f;

	// 판정 피드.
	std::array<Entity, kFeedPoolSize> mFeedPool{};
	std::array<bool, kFeedPoolSize> mFeedPoolUsed{};
	std::deque<JudgementEntry> mFeedEntries; // front = 최신
};
