#pragma once
#include "UIFeature.h"
#include "UIComponent.h"
#include "Entity.h"


// 결과판 행 종류
enum class ResultBoardRow : uint8
{
	Result = 0,
	Kills,
	Assist,
	MaxCombo,
	Time,
	Count,
};

// 결과 점수 구간별 랭크
enum class ResultRank : uint8
{
	S = 0,
	A,
	B,
	C,
	F,
	Count,
};


class UIResultBoardFeature : public UIFeature
{
public:
	void Initialize(World* world) override;
	void Update(float dt) override;

private:
	struct RowState
	{
		Entity mBarEntity = NULL_ENTITY;
		Entity mTextEntity = NULL_ENTITY;
		Entity mRankEntity = NULL_ENTITY; // 항목별 등급 문자 (바 오른쪽 옆)
		int32 mFinalValue = 0;      // Time 행은 총 초(sec)
		bool mSpinStarted = false;  // 스핀 시작 SFX 1회용
		bool mLocked = false;       // 최종값 확정 여부
		bool mRankStamped = false;  // 랭크 도장 첫 프레임 1회용
		float mPopElapsed = -1.0f;  // 락 팝 경과(음수면 미시작/완료)
	};

	void EnsureEntities();
	void StartPresentation(uint8 phaseRaw);
	void HideAll();
	void UpdateRow(size_t index, float dt, bool spinTick);

	// Host 전용 결정 버튼 (다시 시작 / 비행정으로)
	void EnsureDecisionButtons();
	void ShowDecisionButtons(UIRenderGroup group);
	void HideDecisionButtons();
	void OnDecisionClicked(bool restart);

	// 항목별 랭크 도장 — 행 락 후 딜레이를 두고 등급 문자를 행 옆에 찍는다.
	// rankLocal: 도장 시작 시각 기준 로컬 시간(음수면 아직).
	void UpdateRowRank(size_t index, float rankLocal);
	static ResultRank ComputeRankFor(ResultBoardRow row, int32 value, bool isFail);
	void TriggerCameraShake();

	// Tab 홀드 - 인게임 점수판
	void UpdateQuickView(float dt);
	void SetRenderGroupAll(UIRenderGroup group);

	// 행 순서(RESULT/KILLS/ASSIST/MAXCOMBO/TIME)대로 현재 스냅샷 값 읽기
	void ReadLiveValues(std::array<int32, static_cast<size_t>(ResultBoardRow::Count)>& outValues) const;

	// 스핀 중 표시할 랜덤 값 생성
	int32 MakeSpinValue(int32 finalValue);
	std::wstring FormatRowValue(ResultBoardRow row, int32 value) const;
	void RequestSfx(const char* key);

private:
	static constexpr size_t kRowCount = static_cast<size_t>(ResultBoardRow::Count);

	// 연출 타이밍.
	static constexpr float kRowStagger = 0.35f;      // 행 간 등장 간격
	static constexpr float kFadeDuration = 0.3f;     // 행 페이드인 길이
	static constexpr float kSpinDuration = 1.0f;     // 행 등장 후 룰렛 지속 시간
	static constexpr float kSpinTickInterval = 0.05f;// 룰렛 숫자 갱신 간격
	static constexpr float kPopDuration = 0.18f;     // 락 팝 길이
	static constexpr float kPopStartScale = 1.6f;    // 락 순간 텍스트 시작 스케일
	static constexpr float kSlideOffsetX = 60.0f;    // 화면 우측 밖에서 들어오는 슬라이드 오프셋

	// 항목별 랭크 도장 타이밍/연출.
	static constexpr float kRankStartDelay = 0.2f;    // 각 행 락 후 대기
	static constexpr float kRankStampDuration = 0.3f; // 도장 수렴 길이
	static constexpr float kRankStartScale = 1.8f;    // 도장 시작 스케일

	// Tab용
	static constexpr float kQuickRowStagger = 0.05f;   // 행 간 등장 간격
	static constexpr float kQuickFadeDuration = 0.12f; // 행 페이드인 길이
	static constexpr float kQuickSlideOffsetX = 30.0f; // 짧은 슬라이드 오프셋 (우측에서)

	std::array<RowState, kRowCount> mRows{};
	bool mActive = false;
	float mElapsed = 0.0f;
	float mSpinTickAccum = 0.0f;

	bool mIsFail = false; // Fail 페이즈 여부 (true면 RESULT 항목 랭크 F 고정)

	// Host 전용 결정 버튼 상태
	Entity mRestartButton = NULL_ENTITY;
	Entity mLeaveButton = NULL_ENTITY;
	bool mDecisionShown = false;
	bool mDecisionSent = false;   // 중복 요청 방지
	bool mSavedMouseLook = false; // 결과 화면 진입 전 마우스 룩 상태 (복귀 시 복원)
	bool mCursorReleased = false;

	// tab 상태
	bool mQuickActive = false;
	float mQuickElapsed = 0.0f;

};
