#include "pch.h"
#include "UIGameInfoUpdateFeature.h"

#include "Engine.h"
#include "GameMode.h"
#include "TransformComponent.h"
#include "RenderManager.h"
#include "ResourceManager.h"
#include "Mesh.h"
#include "Shader.h"
#include "Texture.h"
#include "World.h"
#include "CameraComponent.h"
#include "TagComponent.h"
#include "UIComponent.h"
#include "UIRenderSystem.h"
#include "UITransformComponent.h"
#include "UISpriteComponent.h"
#include "UITextComponent.h"
#include "GameRuleComponent.h"

#include "MathUtils.h"


void UIGameInfoUpdateFeature::Initialize(World* world)
{
	UIFeature::Initialize(world);

	mTable = {
		{ WavePhaseType::Prepare,
			{ L"UI_Ingame_Conquest",  L"",  Vec2(640.f, 200.f), Vec2(0.f, -380.f), 0.6f, 3.0f, 0.5f } },
		{ WavePhaseType::Conquest,
			{ L"UI_Ingame_Conquest", L"", Vec2(640.f, 200.f), Vec2(0.f, -580.f), 0.6f, 3.0f, 0.5f } },
	/*	{ WavePhaseType::Escort,
			{ L"UI_Goal_Escort",   L"",   Vec2(640.f, 200.f), Vec2(0.f, -380.f), 0.6f, 1.0f, 0.5f } },
		{ WavePhaseType::Boss,
			{ L"UI_Goal_Boss",     L"",     Vec2(640.f, 200.f), Vec2(0.f, -380.f), 0.6f, 1.0f, 0.5f } },
		{ WavePhaseType::Clear,
			{ L"UI_Goal_Clear",    L"CLEAR",    Vec2(640.f, 200.f), Vec2(0.f, -380.f), 0.6f, 1.0f, 0.5f } },*/
	};
}

void UIGameInfoUpdateFeature::Update(float dt)
{
	Entity e = mWorld->GetGameRuleEntity();
	GameRuleComponent* gameRuleComp = mWorld->GetComponent<GameRuleComponent>(e);
	if (!gameRuleComp) return;
	mTotalGameTime = gameRuleComp->mGameTime;

	// phase 전환 감지
	const WavePhaseType newPhase = WavePhaseType(gameRuleComp->mGamePhase);
	if (newPhase != mCurrentPhase)
	{
		mCurrentPhase = newPhase;
		OnPhaseChanged(newPhase);
	}

	// phase 와 무관한 단일 상태머신 tick.
	TickGoalBanner(dt);

	// phase 별 지속 표시(점수/타이머/체크포인트 등)는 종전대로.
	switch (gameRuleComp->mGamePhase)
	{
	case uint8(WavePhaseType::Prepare):
		UpdatePreparePhase(dt, gameRuleComp);
		break;
	case uint8(WavePhaseType::Conquest):
		UpdateConquestPhase(dt, gameRuleComp);
		break;
	case uint8(WavePhaseType::Escort):
		UpdateEscortPhase(dt, gameRuleComp);
		break;
	}
}



void UIGameInfoUpdateFeature::WorldRender(CameraComponent* camera)
{
}

void UIGameInfoUpdateFeature::SpriteRender(DirectX::SpriteBatch* spriteBatch)
{

}

void UIGameInfoUpdateFeature::CustomSpriteRender(std::vector<UIInstanceData>& uiInstanceData)
{
}

void UIGameInfoUpdateFeature::PostSpriteRender(std::vector<UIInstanceData>& uiInstanceData)
{
}

// 각 게임 단계별 업데이트 함수 구현

void UIGameInfoUpdateFeature::UpdatePreparePhase(float dt, GameRuleComponent* gameRuleComp)
{
}

void UIGameInfoUpdateFeature::UpdateConquestPhase(float dt, GameRuleComponent* gameRuleComp)
{

}

void UIGameInfoUpdateFeature::UpdateEscortPhase(float dt, GameRuleComponent* gameRuleComp)
{
}

// 게임 정보 렌더링 함수 구현

void UIGameInfoUpdateFeature::RenderGameTime(CameraComponent* camera)
{
}

void UIGameInfoUpdateFeature::RenderPlayerScore(CameraComponent* camera)
{
}

void UIGameInfoUpdateFeature::RenderMainGoal(CameraComponent* camera)
{
}



// Goal Banner: 데이터 테이블 + 단일 상태머신

void UIGameInfoUpdateFeature::OnPhaseChanged(WavePhaseType newPhase)
{
	const auto& table = GetPhaseTable();
	auto it = table.find(newPhase);
	if (it == table.end())
	{
		// 테이블에 등록되지 않은 phase(None 등) — 엔티티는 유지하되 숨김.
		SetBannerVisible(false);
		mBannerState  = GoalBannerState::Idle;
		mStateElapsed = 0.f;
		return;
	}

	mActiveSpec   = it->second;
	mStateElapsed = 0.f;

	// 엔티티는 최초 1회만 생성되고 이후엔 spec 만 갈아끼움.
	EnsureBannerEntities();
	ApplyBannerSpec(mActiveSpec);
	SetBannerPosition(mCenterPos);
	SetBannerAlpha(0.f);
	SetBannerVisible(true);

	mBannerState = GoalBannerState::FadingIn;
}

void UIGameInfoUpdateFeature::TickGoalBanner(float dt)
{
	if (mBannerState == GoalBannerState::Idle) return;
	if (mBannerEntity == NULL_ENTITY)
	{
		mBannerState = GoalBannerState::Idle;
		return;
	}

	mStateElapsed += dt;
	const PhaseGoalSpec& s = mActiveSpec;

	switch (mBannerState)
	{
	case GoalBannerState::FadingIn:
	{
		const float t = (s.mFadeInDuration > 0.f)
			? std::min(mStateElapsed / s.mFadeInDuration, 1.f): 1.f;
		SetBannerAlpha(t);
		if (t >= 1.f)
		{
			mBannerState  = GoalBannerState::Holding;
			mStateElapsed = 0.f;
		}
		break;
	}
	case GoalBannerState::Holding:
	{
		if (mStateElapsed >= s.mHoldDuration)
		{
			mBannerState  = GoalBannerState::MovingToTop;
			mStateElapsed = 0.f;
		}
		break;
	}
	case GoalBannerState::MovingToTop:
	{
		const float t = (s.mMoveDuration > 0.f)
			? std::min(mStateElapsed / s.mMoveDuration, 1.f): 1.f;
		const float k = EaseOutCubic(t);
		SetBannerPosition(Vec2::Lerp(mCenterPos, s.mTopAnchorPos, k));
		if (t >= 1.f)
		{
			SetBannerPosition(s.mTopAnchorPos);
			mBannerState  = GoalBannerState::Anchored;
			mStateElapsed = 0.f;
		}
		break;
	}
	case GoalBannerState::Anchored:
		// 다음 OnPhaseChanged 까지 그대로 유지.
		break;

	case GoalBannerState::Idle:
	default:
		break;
	}
}

void UIGameInfoUpdateFeature::EnsureBannerEntities()
{
	// 배너 본체
	if (mBannerEntity == NULL_ENTITY)
	{
		mBannerEntity = mWorld->CreateEntity();

		auto& tr = mWorld->AddComponent<UITransformComponent>(mBannerEntity);
		tr.mAnchor       = Anchor::Center;
		tr.mPivot        = Vec2(0.5f, 0.5f);
		tr.mSize         = Vec2(0.f, 0.f);
		tr.mPosition     = mCenterPos;
		tr.mUILayerIndex = 8; // 일반 HUD 위.

		auto& spr = mWorld->AddComponent<UISpriteComponent>(mBannerEntity);
		spr.mVisible   = false;
		spr.mColorTint = Vec4(1.f, 1.f, 1.f, 0.f);
	}

	// 배너 텍스트 엔티티: 빈 문자열이면 visible=false.
	if (mBannerTextEntity == NULL_ENTITY)
	{
		mBannerTextEntity = mWorld->CreateEntity();

		auto& tt = mWorld->AddComponent<UITransformComponent>(mBannerTextEntity);
		tt.mAnchor       = Anchor::Center;
		tt.mPivot        = Vec2(0.5f, 0.5f);
		tt.mSize         = Vec2(0.f, 0.f);
		tt.mPosition     = mCenterPos;
		tt.mUILayerIndex = 9; // 배너 위에 텍스트.

		auto& txt = mWorld->AddComponent<UITextComponent>(mBannerTextEntity);
		txt.mFontType = UIFontType::Esamanru;
		txt.mText.clear();
	}
}

void UIGameInfoUpdateFeature::ApplyBannerSpec(const PhaseGoalSpec& spec)
{
	// 본체 스프라이트
	if (auto* tr = mWorld->GetComponent<UITransformComponent>(mBannerEntity))
		tr->mSize = spec.mBannerSize;

	if (auto* spr = mWorld->GetComponent<UISpriteComponent>(mBannerEntity))
	{
		spr->mTexture = RESOURCEMANAGER.Get<Texture>(spec.mTextureName);
	}

	// 텍스트
	if (auto* tt = mWorld->GetComponent<UITransformComponent>(mBannerTextEntity))
		tt->mSize = spec.mBannerSize;

	if (auto* txt = mWorld->GetComponent<UITextComponent>(mBannerTextEntity))
		txt->mText = spec.mTitleText;
}

void UIGameInfoUpdateFeature::SetBannerVisible(bool visible)
{
	if (mBannerEntity != NULL_ENTITY)
	{
		if (auto* spr = mWorld->GetComponent<UISpriteComponent>(mBannerEntity))
			spr->mVisible = visible;
	}
	// 텍스트도 필요시에 mVisible 추가 후 여기서 함께 토글.
}

void UIGameInfoUpdateFeature::SetBannerAlpha(float alpha)
{
	if (mBannerEntity != NULL_ENTITY)
	{
		if (auto* spr = mWorld->GetComponent<UISpriteComponent>(mBannerEntity))
			spr->mColorTint.w = alpha;
	}
	// 텍스트 알파 제어용 필드는 현재 UITextComponent 에 없음 — 필요 시 확장.
}

void UIGameInfoUpdateFeature::SetBannerPosition(const Vec2& pos)
{
	if (mBannerEntity != NULL_ENTITY)
	{
		if (auto* tr = mWorld->GetComponent<UITransformComponent>(mBannerEntity))
			tr->mPosition = pos;
	}
	if (mBannerTextEntity != NULL_ENTITY)
	{
		if (auto* tt = mWorld->GetComponent<UITransformComponent>(mBannerTextEntity))
			tt->mPosition = pos;
	}
}
