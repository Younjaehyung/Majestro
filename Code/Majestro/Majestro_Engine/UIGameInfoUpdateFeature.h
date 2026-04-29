#pragma once
#include "UIComponent.h"
#include "UIFeature.h"
#include "Entity.h"

class GameRuleComponent;

// Phase 전환 시 띄울 목표 배너 한 장에 대한 정적 사양.
struct PhaseGoalSpec
{
	std::wstring mTextureName;                 // RESOURCEMANAGER.Get<Texture> 키
	std::wstring mTitleText;                   // 배너 안에 그릴 텍스트(빈 문자열이면 스킵)
	Vec2         mBannerSize     = { 640.f, 200.f };
	Vec2         mTopAnchorPos   = { 0.f, -380.f }; // Anchor::Center 기준, 상단에 정착할 위치
	float        mFadeInDuration = 0.6f;	   // 중앙에서 페이드인하는 시간
	float        mHoldDuration   = 1.0f;       // 중앙에서 머무르는 시간
	float        mMoveDuration   = 0.5f;       // 상단으로 이동하는 시간
};

enum class GoalBannerState : uint8
{
	Idle,        // 비활성 — 다음 phase 변경을 기다림
	FadingIn,    // 중앙에서 알파 0 → 1
	Holding,     // 중앙에서 mHoldDuration 만큼 머무름
	MovingToTop, // 중앙에서 spec.mTopAnchorPos 까지 이동
	Anchored,    // 상단에 정착, 다음 phase 변경 전까지 유지
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

	void RenderGameTime(CameraComponent* camera);
	void RenderPlayerScore(CameraComponent* camera);
	void RenderMainGoal(CameraComponent* camera);

	// Phase 전환 시 1회 호출 — 테이블 lookup, 엔티티 준비
	void OnPhaseChanged(WavePhaseType newPhase);

	// 매 프레임 호출 — phase 무관한 단일 상태머신
	void TickGoalBanner(float dt);

	// 배너 엔티티는 한 번만 만들어 재활용한다.
	// EnsureBannerEntities: 최초 1회 엔티티 생성(이후 호출은 noop). visible=false 로 시작.
	// ApplyBannerSpec    : 텍스처/텍스트/사이즈만 spec 으로 교체. 새로 만들지 않음.
	// SetBannerVisible   : UISprite/UIText 의 mVisible 토글.
	void EnsureBannerEntities();
	void ApplyBannerSpec(const PhaseGoalSpec& spec);
	void SetBannerVisible(bool visible);

	// 배너 시각 속성 조작 (엔티티 안전성 체크 포함)
	void SetBannerAlpha(float alpha);
	void SetBannerPosition(const Vec2& pos);

	std::unordered_map<WavePhaseType, PhaseGoalSpec>& GetPhaseTable() {return mTable;}

private:
	WavePhaseType mCurrentPhase = WavePhaseType::None;
	float mTotalGameTime = 0.f;

	// Goal Banner 상태머신
	GoalBannerState mBannerState   = GoalBannerState::Idle;
	PhaseGoalSpec   mActiveSpec;
	std::unordered_map<WavePhaseType, PhaseGoalSpec> mTable;
	float           mStateElapsed  = 0.f;
	Vec2            mCenterPos     = Vec2(0.f, 0.f); // Anchor::Center 기준 정중앙
	Entity          mBannerEntity     = NULL_ENTITY;
	Entity          mBannerTextEntity = NULL_ENTITY;
};

