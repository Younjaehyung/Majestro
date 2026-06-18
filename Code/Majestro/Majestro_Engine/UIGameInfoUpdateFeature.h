#pragma once
#include "UIComponent.h"
#include "UIFeature.h"
#include "Entity.h"

class GameRuleComponent;

// 진입(Intro) 연출 종류 — 등장 시점에서만 동작.
enum class IntroAnim : uint8
{
	None,
	FadeInCenter,    // 중앙 고정
	PopScale,        // 스케일 0.5  오버슈트  1.0
	ZoomBurst,       // 스케일 큰 값 -> 1.0 + 1프레임 플래시
	SlideInLeft,     // 화면 밖 좌측에서 슬라이드인
	SlideInRight,    // 화면 밖 우측에서 슬라이드인
	DropFromTop,     // 위에서 떨어져 감쇠 바운스
	Flash,           // 풀 알파 + 큰 스케일 짧게 -> 즉시 페이드 시작
};

// 정착(Settle/Hold) 연출 종류 — 등장 후 유지 단계에서 동작.
// MoveToTop 은 Settle 단계에서 1회 트랜지션, 나머지는 Hold 단계 동안 지속.
enum class SettledAnim : uint8
{
	Static,          // 그대로 유지
	MoveToTop,       // 중앙에서 mTopAnchorPos 까지 1회 이동 (mSettleDuration)
	BeatPulse,       // EvBeat 수신 시 스케일 펄스 (지속)
	Shake,           // 위치 진동 (지속)
};

// 퇴장(Outro) 연출 종류. None 이면 다음 phase 변경 전까지 유지.
enum class OutroAnim : uint8
{
	None,
	FadeOut,
	SlideOutLeft,
	SlideOutRight,
	ScaleOut,
};

// 연출별 파라미터 일괄.
struct UiAnimParams
{
	float mIntroDuration  = 0.6f;
	float mSettleDuration = 0.5f;   // MoveToTop 등 1회 정착 동작 길이
	float mHoldDuration   = 1.0f;   // 정착 후 유지 시간 (Outro=None 이면 무시)
	float mOutroDuration  = 0.4f;

	Vec2  mTopAnchorPos    = { 0.f, -380.f };   // MoveToTop 도착 위치 (Anchor::Center 기준)
	Vec2  mSlideOffset     = { 1200.f, 0.f };   // SlideIn/Out 시작/도착 오프셋
	Vec2  mDropFromOffset  = { 0.f, -800.f };   // DropFromTop 시작 오프셋

	float mPulseAmplitude  = 0.05f;             // BeatPulse: -1.0 ~ 1.0
	float mPulseDecay      = 6.f;               // BeatPulse 감쇠 속도
	float mShakeAmplitude  = 8.f;               // Shake 픽셀 진폭
	float mShakeFrequency  = 25.f;
	float mOvershootScale  = 1.1f;              // PopScale 오버슈트
	float mZoomBurstStart  = 2.0f;              // ZoomBurst 시작 스케일
	float mFlashScale      = 1.5f;              // Flash 1프레임 스케일
	float mFlashDuration   = 0.08f;             // Flash 의 풀-알파 유지 시간
	float mDropBounceFreq  = 6.f;
	float mDropBounceDamp  = 4.f;
};

// Phase 전환 시 띄울 목표 배너 한 장에 대한 정적 사양.
struct PhaseGoalSpec
{
	std::wstring mTextureName;                  // RESOURCEMANAGER.Get<Texture> 키
	std::wstring mTitleText;                    // 빈 문자열이면 텍스트 미표시
	Vec2         mBannerSize = { 640.f, 200.f };
	// true 면 ScreenRatio(1,1) 레이아웃으로 종횡비 무관 전체화면 (mBannerSize 무시).
	bool         mFullscreen = false;

	IntroAnim    mIntro   = IntroAnim::FadeInCenter;
	SettledAnim  mSettled = SettledAnim::MoveToTop;
	OutroAnim    mOutro   = OutroAnim::None;
	UiAnimParams   mParams;
};

// 배너 상태머신 단계.
// 진행 흐름: Idle -> Intro -> Settle(옵션) -> Hold -> Outro(옵션) -> Done
//   - Settle 은 SettledAnim::MoveToTop 같은 1회 트랜지션이 있을 때만 사용.
//   - Outro = None 이면 Hold 에서 정지 (다음 phase 변경까지 유지).
enum class BannerStage : uint8
{
	Idle,
	Intro,
	Settle,
	Hold,
	Outro,
	Done,
};

// Compute*Visual 들이 채워서 ApplyVisual 한 번에 적용.
struct BannerVisual
{
	Vec2  mPosition = { 0.f, 0.f };
	Vec2  mScale    = { 1.f, 1.f };
	float mAlpha    = 1.f;
};

// 좌측부터 드러나는 배경과 중앙 Stamp를 조합한 재사용 가능한 Phase 연출 설정.
struct RevealStampAnimationSpec
{
	std::wstring mRevealTextureName;
	std::wstring mStampTextureName;

	float mRevealDuration = 0.9f;
	float mStampStartTime = 0.75f;
	float mStampDuration = 0.32f;
	float mStampStartScale = 1.65f;

	uint8 mRevealLayer = 8;
	uint8 mStampLayer = 9;

	Vec3 mCameraShakeAngles = Vec3(1.2f, 0.8f, 1.8f);
	float mCameraShakeDuration = 0.22f;
	float mCameraShakeFrequency = 24.0f;
};

// 연출 엔티티와 타이머를 한곳에서 관리해 다른 Phase에서도 같은 실행기를 재사용한다.
struct RevealStampAnimationState
{
	RevealStampAnimationSpec mSpec;
	Entity mRevealEntity = NULL_ENTITY;
	Entity mStampEntity = NULL_ENTITY;
	float mElapsed = 0.0f;
	bool mActive = false;
	bool mStampTriggered = false;
};

class UIGameInfoUpdateFeature : public UIFeature
{
public:
	void Initialize(World* world);

	void Update(float dt);
	void WorldRender(CameraComponent* camera);
	void SpriteRender(DirectX::SpriteBatch* /*spriteBatch*/);
	void CustomSpriteRender(std::vector<UIInstanceData>& instances);
	void PostSpriteRender(std::vector<UIInstanceData>& instances);

private:
	void UpdatePreparePhase(float dt, GameRuleComponent* gameRuleComp);
	void UpdateConquestPhase(float dt, GameRuleComponent* gameRuleComp);
	void UpdateEscortPhase(float dt, GameRuleComponent* gameRuleComp);
	void UpdateClearPhase(float dt, GameRuleComponent* gameRuleComp);

	void RenderGameTime(CameraComponent* camera);
	void RenderPlayerScore(CameraComponent* camera);
	void RenderMainGoal(CameraComponent* camera);

	// Phase 전환 시 1회 호출 — 테이블 lookup, spec 적용, 상태머신 초기화.
	void OnPhaseChanged(WavePhaseType newPhase);

	// 매 프레임 호출 — stage 진행/전이 + 시각 속성 계산/적용.
	void TickGoalBanner(float dt);
	void TickRevealStampAnimation(float dt);

	// 배너 엔티티 (재활용).
	void EnsureBannerEntities();
	void EnsureRevealStampEntities();
	void EnsurePhaseStatusText();
	void ApplyBannerSpec(const PhaseGoalSpec& spec);
	void SetBannerVisible(bool visible);
	void SetPhaseStatusVisible(bool visible);
	void StartRevealStampAnimation(const RevealStampAnimationSpec& spec);
	void StopRevealStampAnimation();
	void TriggerAnimationCameraShake(const RevealStampAnimationSpec& spec);

	// 시각 속성을 BannerVisual 한 번에 엔티티 컴포넌트로 적용.
	void ApplyVisual(const BannerVisual& v);

	// 단계별 시각 계산. base 는 정착 위치/스케일/알파의 기준점.
	// out 은 base 로 초기화된 상태로 들어와 각 함수가 변조하여 합성.
	void ComputeIntroVisual(IntroAnim type, const UiAnimParams& p, float t,
		const BannerVisual& base, BannerVisual& out) const;
	void ComputeSettleVisual(SettledAnim type, const UiAnimParams& p, float t,
		const BannerVisual& base, BannerVisual& out) const;
	void ComputeHoldVisual(SettledAnim type, const UiAnimParams& p, float dt,
		const BannerVisual& base, BannerVisual& out);
	void ComputeOutroVisual(OutroAnim type, const UiAnimParams& p, float t,
		const BannerVisual& base, BannerVisual& out) const;

	// Settle 단계 종료 시 정착점 갱신용. (예: MoveToTop 후 base.mPosition = topAnchor)
	void UpdateBaseAfterSettle();

	// ImGui 디버그 패널 — phase 강제 전환 및 상태머신 상태 표시.
	void DrawDebugPanel();

	std::unordered_map<WavePhaseType, PhaseGoalSpec>& GetPhaseTable() { return mTable; }

private:
	WavePhaseType mCurrentPhase = WavePhaseType::None;
	float mTotalGameTime = 0.f;

	// 배너 상태머신.
	BannerStage   mStage         = BannerStage::Idle;
	PhaseGoalSpec mActiveSpec;
	std::unordered_map<WavePhaseType, PhaseGoalSpec> mTable;
	float         mStageElapsed  = 0.f;
	Vec2          mCenterPos     = Vec2(0.f, 0.f); // Anchor::Center 기준 정중앙

	// 정착 기준점 — Settle 종료 후 Hold/Outro 가 사용.
	BannerVisual  mBaseVisual;

	// BeatPulse 상태 (지속 동작).
	float         mPulsePhase    = 0.f;            // EvBeat 수신 시 1.f 로 세팅, dt 에 따라 감쇠

	Entity        mBannerEntity     = NULL_ENTITY;
	Entity        mBannerTextEntity = NULL_ENTITY;
	Entity        mPhaseStatusTextEntity = NULL_ENTITY;

	std::unordered_map<WavePhaseType, RevealStampAnimationSpec> mRevealStampAnimationTable;
	RevealStampAnimationState mRevealStampAnimation;
};
