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
#include "PlayerComponent.h"
#include "GameEvents.h"
#include "EventManager.h"

#include "MathUtils.h"
#include <sstream>

void UIGameInfoUpdateFeature::Initialize(World* world)
{
	UIFeature::Initialize(world);


	mTable.clear();
	mRevealStampAnimationTable.clear();
	mRevealStampAnimation = RevealStampAnimationState{};

	// Prepare:
	{
		PhaseGoalSpec s;
		s.mTextureName = L"UI_Ingame_Prepare";
		s.mIntro   = IntroAnim::FadeInCenter;
		s.mSettled = SettledAnim::MoveToTop;
		s.mOutro   = OutroAnim::None;
		s.mParams.mIntroDuration  = 0.6f;
		s.mParams.mHoldDuration   = 3.0f;
		s.mParams.mSettleDuration = 0.5f;
		s.mParams.mTopAnchorPos   = Vec2(0.f, -380.f);
		mTable[WavePhaseType::Prepare] = s;
	}

	// Conquest:
	{
		PhaseGoalSpec s;
		s.mTextureName = L"UI_Ingame_Conquest";
		s.mIntro   = IntroAnim::FadeInCenter;
		s.mSettled = SettledAnim::MoveToTop;
		s.mOutro   = OutroAnim::None;
		s.mParams.mIntroDuration  = 0.6f;
		s.mParams.mHoldDuration   = 3.0f;
		s.mParams.mSettleDuration = 0.5f;
		s.mParams.mTopAnchorPos   = Vec2(0.f, -580.f);
		// MoveToTop 정착 후에도 펄스가 가능하도록 BeatPulse 와 합성하려면
		// Hold 단계에서 별도 동작 필요 — 현 구조에선 Settled 가 MoveToTop 이면
		// 정착 후 Hold 단계의 동작은 Static 이 된다.
		// Conquest 에서 비트 펄스를 항시 원하면 mSettled = BeatPulse 로 두고
		// mTopAnchorPos 대신 처음부터 상단에서 등장시키는 식으로 바꿀 수 있음.
		mTable[WavePhaseType::Conquest] = s;
	}

	// Escort:
	{
		PhaseGoalSpec s;
		s.mTextureName = L"UI_Ingame_Escort";
		s.mIntro   = IntroAnim::SlideInLeft;
		s.mSettled = SettledAnim::Static;
		s.mOutro   = OutroAnim::SlideOutRight;
		s.mParams.mIntroDuration = 0.5f;
		s.mParams.mHoldDuration  = 1.5f;
		s.mParams.mOutroDuration = 0.5f;
		s.mParams.mSlideOffset   = Vec2(1500.f, 0.f);
		mTable[WavePhaseType::Escort] = s;
	}

	// Boss
	{
		PhaseGoalSpec s;
		s.mTextureName = L"UI_Ingame_Killboss";
		s.mIntro   = IntroAnim::ZoomBurst;
		s.mSettled = SettledAnim::Shake;
		s.mOutro   = OutroAnim::FadeOut;
		s.mParams.mIntroDuration = 0.4f;
		s.mParams.mHoldDuration  = 1.0f;
		s.mParams.mOutroDuration = 0.5f;
		s.mParams.mZoomBurstStart = 2.0f;
		s.mParams.mShakeAmplitude = 10.f;
		s.mParams.mShakeFrequency = 22.f;
		mTable[WavePhaseType::Boss] = s;
	}

	// Fail:
	{
		PhaseGoalSpec s;
		s.mTextureName = L"UI_Ingame_Fail";
		s.mIntro   = IntroAnim::PopScale;
		s.mSettled = SettledAnim::Shake;
		s.mOutro   = OutroAnim::FadeOut;
		s.mParams.mIntroDuration = 0.3f;
		s.mParams.mHoldDuration  = 0.8f;
		s.mParams.mOutroDuration = 0.5f;
		s.mParams.mShakeAmplitude = 12.f;
		s.mParams.mShakeFrequency = 28.f;
		mTable[WavePhaseType::Fail] = s;
	}

	// Phase별 Reveal과 Stamp 연출은 이 테이블에 설정만 추가해 재사용한다.
	{
		RevealStampAnimationSpec clearSpec;
		clearSpec.mRevealTextureName = L"UI_StageClear_1";
		clearSpec.mStampTextureName = L"UI_StageClear_2";
		if (mWorld->GetSceneId() == SceneId::ThirdGame)
			clearSpec.mFinalStampTextureName = L"UI_GameClear";
		clearSpec.mRevealDuration = 0.9f;
		clearSpec.mStampStartTime = 0.75f;
		clearSpec.mStampDuration = 0.32f;
		clearSpec.mStampStartScale = 1.65f;
		clearSpec.mFinalStampStartTime = 1.45f;
		clearSpec.mFinalStampDuration = 0.42f;
		clearSpec.mFinalStampStartScale = 1.5f;
		clearSpec.mRevealLayer = 8;
		clearSpec.mStampLayer = 9;
		clearSpec.mCameraShakeAngles = Vec3(1.2f, 0.8f, 1.8f);
		clearSpec.mCameraShakeDuration = 0.22f;
		clearSpec.mCameraShakeFrequency = 24.0f;
		mRevealStampAnimationTable[WavePhaseType::Clear] = clearSpec;

		// Fail 페이즈에서도 GameClear와 완전히 같은 타이밍과 카메라 연출을 재사용한다.
		RevealStampAnimationSpec gameOverSpec = clearSpec;
		gameOverSpec.mRevealTextureName = L"UI_GameOver_1";
		gameOverSpec.mStampTextureName = L"UI_GameOver_2";
		gameOverSpec.mFinalStampTextureName.clear();
		mRevealStampAnimationTable[WavePhaseType::Fail] = gameOverSpec;
	}
}

void UIGameInfoUpdateFeature::Update(float dt)
{
	GameRuleComponent* gameRuleComp = mWorld->GetSingleton<GameRuleComponent>();
	if (!gameRuleComp) return;
	mTotalGameTime = gameRuleComp->mGameTime;

	// EvBeat 소비 — BeatPulse 트리거.
	mWorld->GetEventManager()->Consume<EvBeat>([this](const EvBeat&) {
		mPulsePhase = 1.f;
	});

	// phase 전환 감지.
	const WavePhaseType newPhase = WavePhaseType(gameRuleComp->mGamePhase);
	if (newPhase != mCurrentPhase)
	{
		mCurrentPhase = newPhase;
		OnPhaseChanged(newPhase);
	}

	// 단일 상태머신 tick.
	TickGoalBanner(dt);
	TickRevealStampAnimation(dt);

	// 디버그 패널은 Render phase 에서 호출해야 함 (NewFrame 이후).
	// → PostSpriteRender 에서 호출.

	// phase 별 지속 표시 (점수/타이머 등) — 기존 코드 유지.
	SetPhaseStatusVisible(false);
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
	case uint8(WavePhaseType::Clear):
		UpdateClearPhase(dt, gameRuleComp);
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
#ifdef _IMGUI
	// UIRenderSystem (Render phase) 가 호출하는 시점 — ImGui::NewFrame 이후라 안전.
	DrawDebugPanel();
#endif
}

// 각 게임 단계별 업데이트 함수 구현

void UIGameInfoUpdateFeature::UpdatePreparePhase(float dt, GameRuleComponent* gameRuleComp)
{
	GamePrepareComponent* prepareComp = mWorld->GetSingleton<GamePrepareComponent>();
	if (!prepareComp)
		return;

	EnsurePhaseStatusText();
	SetPhaseStatusVisible(true);

	if (UITransformComponent* transform = mWorld->GetComponent<UITransformComponent>(mPhaseStatusTextEntity))
		transform->mPosition = Vec2(0.0f, -250.0f);

	UITextComponent* text = mWorld->GetComponent<UITextComponent>(mPhaseStatusTextEntity);
	if (!text)
		return;

	std::wstringstream stream;
	stream << L"Players Ready: " << prepareComp->mReadyPlayers
		<< L" / " << prepareComp->mTotalPlayers << L"\n";

	if (prepareComp->mCountdownStarted)
	{
		if (prepareComp->mAllPlayersReady)
			stream << L"All Players Ready\n";

		stream << L"Game Starts In: "
			<< static_cast<int32>(std::ceil(prepareComp->mCountdownRemaining));
	}
	else if (prepareComp->mReadyCheckRemaining > 0.0f)
	{
		stream << L"Ready Check In: "
			<< static_cast<int32>(std::ceil(prepareComp->mReadyCheckRemaining));
	}
	else
	{
		stream << L"Force Start In: "
			<< static_cast<int32>(std::ceil(prepareComp->mForcedStartRemaining));
	}

	text->mText = stream.str();
}

void UIGameInfoUpdateFeature::UpdateConquestPhase(float dt, GameRuleComponent* gameRuleComp)
{

}

void UIGameInfoUpdateFeature::UpdateEscortPhase(float dt, GameRuleComponent* gameRuleComp)
{
}

void UIGameInfoUpdateFeature::UpdateClearPhase(float dt, GameRuleComponent* gameRuleComp)
{
	GameClearComponent* clearComp = mWorld->GetSingleton<GameClearComponent>();
	if (!clearComp)
		return;

	// Keep the result table visible before and after the final clear image appears.
	EnsurePhaseStatusText();
	SetPhaseStatusVisible(true);

	if (UITransformComponent* transform = mWorld->GetComponent<UITransformComponent>(mPhaseStatusTextEntity))
	{
		// Layer 10 keeps the score board above the final clear image on layer 9.
		transform->mPosition = Vec2(0.0f, 120.0f);
		transform->mSize = Vec2(1000.0f, 520.0f);
	}

	auto playerTypeName = [](uint8 playerType) -> const wchar_t*
	{
		switch (static_cast<PlayerType>(playerType))
		{
		case PlayerType::Rudwig: return L"Rudwig";
		case PlayerType::Ibanix: return L"Ibanix";
		case PlayerType::Fanthor: return L"Fanthor";
		default: return L"Unknown";
		}
	};

	UITextComponent* text = mWorld->GetComponent<UITextComponent>(mPhaseStatusTextEntity);
	if (!text)
		return;

	std::wstringstream stream;
	stream << L"SCORE BOARD\n";
	stream << L"RANK  PLAYER  SCORE  KILLS\n";

	ScoreBoardComponent* scoreBoard = mWorld->GetSingleton<ScoreBoardComponent>();
	if (scoreBoard && scoreBoard->mPlayerCount > 0)
	{
		for (uint8 index = 0; index < scoreBoard->mPlayerCount && index < ROOM_MAX_PLAYERS; ++index)
		{
			const ClientPlayerScore& player = scoreBoard->mPlayers[index];
			stream << static_cast<int32>(index + 1) << L".  "
				<< playerTypeName(player.mPlayerType) << L"  "
				<< player.mScore << L"  "
				<< player.mTotalKills << L"\n";
		}
	}
	else
	{
		// Keep the player list visible if the first score board packet has not arrived yet.
		for (uint8 index = 0; index < clearComp->mPlayerCount && index < ROOM_MAX_PLAYERS; ++index)
		{
			stream << static_cast<int32>(index + 1) << L".  "
				<< playerTypeName(clearComp->mPlayerTypes[index]) << L"  0  0\n";
		}
	}

	stream << L"\nTEAM SCORE  " << clearComp->mTeamScore << L"\n";
	stream << L"PLAY TIME  " << static_cast<int32>(clearComp->mGameTime) << L"s";
	text->mText = stream.str();
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



// Goal Banner: 데이터 테이블 + Intro/Settle/Hold/Outro 상태머신.

void UIGameInfoUpdateFeature::OnPhaseChanged(WavePhaseType newPhase)
{
	const auto revealStampIt = mRevealStampAnimationTable.find(newPhase);
	if (revealStampIt != mRevealStampAnimationTable.end())
	{
		SetBannerVisible(false);
		mStage = BannerStage::Idle;
		mStageElapsed = 0.0f;
		StartRevealStampAnimation(revealStampIt->second);
		return;
	}

	StopRevealStampAnimation();

	const auto& table = GetPhaseTable();
	auto it = table.find(newPhase);
	if (it == table.end())
	{
		// 테이블에 등록되지 않은 phase 유지하되 숨김.
		SetBannerVisible(false);
		mStage         = BannerStage::Idle;
		mStageElapsed  = 0.f;
		return;
	}

	mActiveSpec   = it->second;
	mStageElapsed = 0.f;
	mPulsePhase   = 0.f;

	// 정착 기준점 — 일단 중앙. Settle 단계 종료 후 갱신될 수 있음.
	mBaseVisual.mPosition = mCenterPos;
	mBaseVisual.mScale    = Vec2(1.f, 1.f);
	mBaseVisual.mAlpha    = 1.f;

	// spec 만 갈아끼기.
	EnsureBannerEntities();
	ApplyBannerSpec(mActiveSpec);
	SetBannerVisible(true);

	// Intro 가 None 이면 곧장 Settle/Hold 로 진입.
	if (mActiveSpec.mIntro == IntroAnim::None)
	{
		mStage = (mActiveSpec.mSettled == SettledAnim::MoveToTop)
			? BannerStage::Settle : BannerStage::Hold;
	}
	else
	{
		mStage = BannerStage::Intro;
	}
}

void UIGameInfoUpdateFeature::TickRevealStampAnimation(float dt)
{
	if (!mRevealStampAnimation.mActive)
		return;

	mRevealStampAnimation.mElapsed += max(0.0f, dt);
	const RevealStampAnimationSpec& spec = mRevealStampAnimation.mSpec;

	UISpriteComponent* revealSprite =
		mWorld->GetComponent<UISpriteComponent>(mRevealStampAnimation.mRevealEntity);
	UISpriteComponent* stampSprite =
		mWorld->GetComponent<UISpriteComponent>(mRevealStampAnimation.mStampEntity);
	UITransformComponent* stampTransform =
		mWorld->GetComponent<UITransformComponent>(mRevealStampAnimation.mStampEntity);
	if (!revealSprite || !stampSprite || !stampTransform)
		return;

	UISpriteComponent* finalStampSprite = nullptr;
	UITransformComponent* finalStampTransform = nullptr;
	if (!spec.mFinalStampTextureName.empty())
	{
		finalStampSprite =
			mWorld->GetComponent<UISpriteComponent>(mRevealStampAnimation.mFinalStampEntity);
		finalStampTransform =
			mWorld->GetComponent<UITransformComponent>(mRevealStampAnimation.mFinalStampEntity);
		if (!finalStampSprite || !finalStampTransform)
			return;
	}

	// Reveal 레이어는 이동하지 않고 왼쪽부터 오른쪽으로 표시 범위와 알파를 늘린다.
	const float revealDuration = max(spec.mRevealDuration, 0.001f);
	const float revealLinear = std::clamp(
		mRevealStampAnimation.mElapsed / revealDuration, 0.0f, 1.0f);
	const float revealProgress =
		revealLinear * revealLinear * (3.0f - 2.0f * revealLinear);
	revealSprite->SetVisibleRangeNormalizedX(0.0f, revealProgress);
	revealSprite->SetVisibleRangeKeepDestinationSize(true);
	revealSprite->mColorTint.w = revealProgress;

	// Stamp 레이어는 설정된 시점에 큰 스케일로 나타나 원래 크기로 빠르게 수렴한다.
	if (mRevealStampAnimation.mElapsed < spec.mStampStartTime)
	{
		stampSprite->mVisible = false;
		return;
	}

	if (!mRevealStampAnimation.mStampTriggered)
	{
		mRevealStampAnimation.mStampTriggered = true;
		TriggerAnimationCameraShake(spec);
	}

	stampSprite->mVisible = true;
	const float stampDuration = max(spec.mStampDuration, 0.001f);
	const float stampProgress = std::clamp(
		(mRevealStampAnimation.mElapsed - spec.mStampStartTime) / stampDuration,
		0.0f, 1.0f);
	const float eased = 1.0f - std::pow(1.0f - stampProgress, 3.0f);
	const float scale =
		1.0f + (spec.mStampStartScale - 1.0f) * (1.0f - eased);
	stampTransform->mScale = Vec2(scale, scale);
	stampSprite->mColorTint.w = std::clamp(stampProgress * 5.0f, 0.0f, 1.0f);

	if (spec.mFinalStampTextureName.empty() ||
		mRevealStampAnimation.mElapsed < spec.mFinalStampStartTime)
		return;

	if (!mRevealStampAnimation.mFinalStampTriggered)
	{
		// StageClear 도장 연출이 끝난 뒤 별도 전체 화면 레이어에 GameClear 이미지를 한 번 더 찍는다.
		mRevealStampAnimation.mFinalStampTriggered = true;
		finalStampSprite->mVisible = true;
		finalStampTransform->mScale =
			Vec2(spec.mFinalStampStartScale, spec.mFinalStampStartScale);
		finalStampSprite->mColorTint.w = 0.0f;
		TriggerAnimationCameraShake(spec);
	}

	const float finalStampDuration = max(spec.mFinalStampDuration, 0.001f);
	const float finalStampProgress = std::clamp(
		(mRevealStampAnimation.mElapsed - spec.mFinalStampStartTime) / finalStampDuration,
		0.0f, 1.0f);
	const float finalStampEased =
		1.0f - std::pow(1.0f - finalStampProgress, 3.0f);
	const float finalStampScale =
		1.0f +
		(spec.mFinalStampStartScale - 1.0f) * (1.0f - finalStampEased);
	finalStampTransform->mScale = Vec2(finalStampScale, finalStampScale);
	finalStampSprite->mColorTint.w =
		std::clamp(finalStampProgress * 5.0f, 0.0f, 1.0f);
}

void UIGameInfoUpdateFeature::TickGoalBanner(float dt)
{
	if (mStage == BannerStage::Idle || mStage == BannerStage::Done) return;
	if (mBannerEntity == NULL_ENTITY)
	{
		mStage = BannerStage::Idle;
		return;
	}

	mStageElapsed += dt;
	const PhaseGoalSpec& spec = mActiveSpec;
	const UiAnimParams& p = spec.mParams;

	BannerVisual visual = mBaseVisual;

	switch (mStage)
	{
	case BannerStage::Intro:
	{
		const float dur = std::max(p.mIntroDuration, 0.0001f);
		const float t   = std::min(mStageElapsed / dur, 1.f);
		ComputeIntroVisual(spec.mIntro, p, t, mBaseVisual, visual);
		if (t >= 1.f)
		{
			// Intro 종료. MoveToTop 이 있으면 Settle, 아니면 Hold.
			mStageElapsed = 0.f;
			mStage = (spec.mSettled == SettledAnim::MoveToTop)
				? BannerStage::Settle : BannerStage::Hold;
		}
		break;
	}

	case BannerStage::Settle:
	{
		const float dur = std::max(p.mSettleDuration, 0.0001f);
		const float t   = std::min(mStageElapsed / dur, 1.f);
		ComputeSettleVisual(spec.mSettled, p, t, mBaseVisual, visual);
		if (t >= 1.f)
		{
			// 정착 완료 — base 갱신 후 Hold 진입.
			UpdateBaseAfterSettle();
			visual = mBaseVisual;
			mStageElapsed = 0.f;
			mStage = BannerStage::Hold;
		}
		break;
	}

	case BannerStage::Hold:
	{
		ComputeHoldVisual(spec.mSettled, p, dt, mBaseVisual, visual);

		// Outro = None 이면 Hold 에서 정지 (다음 phase 까지 유지).
		if (spec.mOutro == OutroAnim::None)
		{
			// stage 그대로, elapsed 만 누적 — Shake/BeatPulse 위상에 사용.
			break;
		}
		if (mStageElapsed >= p.mHoldDuration)
		{
			mStageElapsed = 0.f;
			mStage = BannerStage::Outro;
		}
		break;
	}

	case BannerStage::Outro:
	{
		const float dur = std::max(p.mOutroDuration, 0.0001f);
		const float t   = std::min(mStageElapsed / dur, 1.f);
		ComputeOutroVisual(spec.mOutro, p, t, mBaseVisual, visual);
		if (t >= 1.f)
		{
			SetBannerVisible(false);
			mStage = BannerStage::Done;
		}
		break;
	}

	default:
		break;
	}

	ApplyVisual(visual);
}

void UIGameInfoUpdateFeature::UpdateBaseAfterSettle()
{
	switch (mActiveSpec.mSettled)
	{
	case SettledAnim::MoveToTop:
		mBaseVisual.mPosition = mActiveSpec.mParams.mTopAnchorPos;
		break;
	default:
		break;
	}
}

void UIGameInfoUpdateFeature::ComputeIntroVisual(IntroAnim type, const UiAnimParams& p, float t,
	const BannerVisual& base, BannerVisual& out) const
{
	out = base;
	switch (type)
	{
		case IntroAnim::None:
			break;

		case IntroAnim::FadeInCenter:
			out.mAlpha = t;
			break;

		case IntroAnim::PopScale:
		{
			// 0.5  오버슈트  1.0. EaseOutBack 으로 자연스러운 오버슈트.
			const float k = EaseOutBack(t);
			const float scale = 0.5f + (1.f - 0.5f) * k;
			// 끝 부분에서 mOvershootScale 까지 살짝 더 부풀어 오르도록 보정.
			const float bump = (k > 1.f) ? (k - 1.f) * (p.mOvershootScale - 1.f) : 0.f;
			const float s = scale + bump;
			out.mScale = Vec2(s, s);
			out.mAlpha = std::min(t * 2.f, 1.f);
			break;
		}

		case IntroAnim::ZoomBurst:
		{
			// 큰 스케일에서 1.0 으로 수렴. EaseOutCubic 으로 후반 감속.
			const float k = EaseOutCubic(t);
			const float s = p.mZoomBurstStart + (1.f - p.mZoomBurstStart) * k;
			out.mScale = Vec2(s, s);
			// 초반 1프레임은 풀 알파, 이후 그대로.
			out.mAlpha = std::min(t * 4.f, 1.f);
			break;
		}

		case IntroAnim::SlideInLeft:
		{
			const float k = EaseOutCubic(t);
			const Vec2 from = base.mPosition - p.mSlideOffset;
			out.mPosition = Vec2::Lerp(from, base.mPosition, k);
			out.mAlpha = 1.f;
			break;
		}

		case IntroAnim::SlideInRight:
		{
			const float k = EaseOutCubic(t);
			const Vec2 from = base.mPosition + p.mSlideOffset;
			out.mPosition = Vec2::Lerp(from, base.mPosition, k);
			out.mAlpha = 1.f;
			break;
		}

		case IntroAnim::DropFromTop:
		{
			// 위에서 떨어져 바운스 — 도착 위치 근처에서 흔들림.
			const float k = EaseOutCubic(t);
			const Vec2 from = base.mPosition + p.mDropFromOffset;
			Vec2 pos = Vec2::Lerp(from, base.mPosition, k);
			// t 후반(0.6 이후) 부터 바운스 추가.
			if (t > 0.6f)
			{
				const float bt = (t - 0.6f) / 0.4f; // 0~1
				const float bounce = DampedSine(bt, p.mDropBounceFreq, p.mDropBounceDamp) * 30.f;
				pos.y += bounce;
			}
			out.mPosition = pos;
			out.mAlpha = std::min(t * 3.f, 1.f);
			break;
		}

		case IntroAnim::Flash:
		{
			// 풀 알파 + 큰 스케일 짧게 유지 이후 스케일/알파 정상화.
			const float flashRatio = (p.mIntroDuration > 0.f)
				? std::min(p.mFlashDuration / p.mIntroDuration, 1.f) : 0.5f;
			if (t < flashRatio)
			{
				out.mScale = Vec2(p.mFlashScale, p.mFlashScale);
				out.mAlpha = 1.f;
			}
			else
			{
				const float k = (t - flashRatio) / std::max(1.f - flashRatio, 0.0001f);
				const float s = p.mFlashScale + (1.f - p.mFlashScale) * EaseOutCubic(k);
				out.mScale = Vec2(s, s);
				out.mAlpha = 1.f;
			}
			break;
		}
	}
}

void UIGameInfoUpdateFeature::ComputeSettleVisual(SettledAnim type, const UiAnimParams& p, float t,
	const BannerVisual& base, BannerVisual& out) const
{
	out = base;
	switch (type)
	{
	case SettledAnim::MoveToTop:
	{
		const float k = EaseOutCubic(t);
		out.mPosition = Vec2::Lerp(base.mPosition, p.mTopAnchorPos, k);
		break;
	}
	// 그 외 SettledAnim 은 Settle 단계를 사용하지 않음.
	default:
		break;
	}
}

void UIGameInfoUpdateFeature::ComputeHoldVisual(SettledAnim type, const UiAnimParams& p, float dt,
	const BannerVisual& base, BannerVisual& out)
{
	out = base;
	switch (type)
	{
	case SettledAnim::Static:
	case SettledAnim::MoveToTop:
		// MoveToTop 은 Settle 종료 후 정착 — Hold 에선 정적.
		break;

	case SettledAnim::BeatPulse:
	{
		// EvBeat 마다 mPulsePhase=1, 시간이 지날수록 감쇠.
		mPulsePhase = std::max(mPulsePhase - dt * p.mPulseDecay, 0.f);
		const float s = 1.f + p.mPulseAmplitude * mPulsePhase;
		out.mScale = Vec2(base.mScale.x * s, base.mScale.y * s);
		break;
	}

	case SettledAnim::Shake:
	{
		const float dx = std::sin(mStageElapsed * p.mShakeFrequency) * p.mShakeAmplitude;
		const float dy = std::cos(mStageElapsed * p.mShakeFrequency * 1.3f) * p.mShakeAmplitude * 0.5f;
		out.mPosition = Vec2(base.mPosition.x + dx, base.mPosition.y + dy);
		break;
	}
	}
}

void UIGameInfoUpdateFeature::ComputeOutroVisual(OutroAnim type, const UiAnimParams& p, float t,
	const BannerVisual& base, BannerVisual& out) const
{
	out = base;
	switch (type)
	{
	case OutroAnim::None:
		break;

	case OutroAnim::FadeOut:
		out.mAlpha = base.mAlpha * (1.f - t);
		break;

	case OutroAnim::SlideOutLeft:
	{
		const float k = EaseInCubic(t);
		const Vec2 to = base.mPosition - p.mSlideOffset;
		out.mPosition = Vec2::Lerp(base.mPosition, to, k);
		break;
	}

	case OutroAnim::SlideOutRight:
	{
		const float k = EaseInCubic(t);
		const Vec2 to = base.mPosition + p.mSlideOffset;
		out.mPosition = Vec2::Lerp(base.mPosition, to, k);
		break;
	}

	case OutroAnim::ScaleOut:
	{
		const float s = (1.f - EaseInCubic(t));
		out.mScale = Vec2(base.mScale.x * s, base.mScale.y * s);
		out.mAlpha = base.mAlpha * (1.f - t);
		break;
	}
	}
}

void UIGameInfoUpdateFeature::ApplyVisual(const BannerVisual& v)
{
	if (mBannerEntity != NULL_ENTITY)
	{
		if (auto* tr = mWorld->GetComponent<UITransformComponent>(mBannerEntity))
		{
			tr->mPosition = v.mPosition;
			tr->mScale    = v.mScale;
		}
		if (auto* spr = mWorld->GetComponent<UISpriteComponent>(mBannerEntity))
		{
			spr->mColorTint.w = v.mAlpha;
		}
	}
	if (mBannerTextEntity != NULL_ENTITY)
	{
		if (auto* tt = mWorld->GetComponent<UITransformComponent>(mBannerTextEntity))
		{
			tt->mPosition = v.mPosition;
			tt->mScale    = v.mScale;
		}
	}
}

void UIGameInfoUpdateFeature::EnsureBannerEntities()
{
	// 배너 본체.
	if (mBannerEntity == NULL_ENTITY)
	{
		mBannerEntity = mWorld->CreateEntity();

		auto& tr = mWorld->AddComponent<UITransformComponent>(mBannerEntity);
		tr.mAnchor       = Anchor::Center;
		tr.mPivot        = Vec2(0.5f, 0.5f);
		tr.mSize         = Vec2(0.f, 0.f);
		tr.mPosition     = mCenterPos;
		tr.mUILayerIndex = 8;

		auto& spr = mWorld->AddComponent<UISpriteComponent>(mBannerEntity);
		spr.mVisible   = false;
		spr.mColorTint = Vec4(1.f, 1.f, 1.f, 0.f);
	}

	// 배너 텍스트 엔티티.
	if (mBannerTextEntity == NULL_ENTITY)
	{
		mBannerTextEntity = mWorld->CreateEntity();

		auto& tt = mWorld->AddComponent<UITransformComponent>(mBannerTextEntity);
		tt.mAnchor       = Anchor::Center;
		tt.mPivot        = Vec2(0.5f, 0.5f);
		tt.mSize         = Vec2(0.f, 0.f);
		tt.mPosition     = mCenterPos;
		tt.mUILayerIndex = 9;

		auto& txt = mWorld->AddComponent<UITextComponent>(mBannerTextEntity);
		txt.mFontType = UIFontType::Esamanru;
		txt.mText.clear();
	}
}

void UIGameInfoUpdateFeature::EnsureRevealStampEntities()
{
	auto createFullscreenLayer = [this](Entity& entity, const std::wstring& textureName, uint8 layer)
	{
		if (entity == NULL_ENTITY)
		{
			entity = mWorld->CreateEntity();

			auto& transform = mWorld->AddComponent<UITransformComponent>(entity);
			transform.mAnchor = Anchor::Center;
			transform.mPivot = Vec2(0.5f, 0.5f);
			transform.UseScreenRatioLayout(Vec2(0.0f, 0.0f), Vec2(1.0f, 1.0f));

			mWorld->AddComponent<UISpriteComponent>(entity);

			// 게임 오버 렌더 필터를 통과시킬 전용 배너 엔티티임을 표시한다.
			// 일반 HUD와 GameWorld UI는 Fail 상태에서 렌더링되지 않는다.
			mWorld->AddComponent<GameOverUIComponent>(entity);
		}

		if (UITransformComponent* transform = mWorld->GetComponent<UITransformComponent>(entity))
			transform->mUILayerIndex = layer;

		UISpriteComponent& sprite = *mWorld->GetComponent<UISpriteComponent>(entity);
		sprite.mTexture = RESOURCEMANAGER.Get<Texture>(textureName);
		sprite.mVisible = false;
		sprite.mColorTint = Vec4(1.0f, 1.0f, 1.0f, 0.0f);
	};

	const RevealStampAnimationSpec& spec = mRevealStampAnimation.mSpec;
	createFullscreenLayer(
		mRevealStampAnimation.mRevealEntity, spec.mRevealTextureName, spec.mRevealLayer);
	createFullscreenLayer(
		mRevealStampAnimation.mStampEntity, spec.mStampTextureName, spec.mStampLayer);
	if (!spec.mFinalStampTextureName.empty())
	{
		createFullscreenLayer(
			mRevealStampAnimation.mFinalStampEntity,
			spec.mFinalStampTextureName,
			255);
	}
}

void UIGameInfoUpdateFeature::StartRevealStampAnimation(const RevealStampAnimationSpec& spec)
{
	StopRevealStampAnimation();

	mRevealStampAnimation.mSpec = spec;
	mRevealStampAnimation.mElapsed = 0.0f;
	mRevealStampAnimation.mActive = true;
	mRevealStampAnimation.mStampTriggered = false;
	mRevealStampAnimation.mFinalStampTriggered = false;
	EnsureRevealStampEntities();

	if (UISpriteComponent* revealSprite =
		mWorld->GetComponent<UISpriteComponent>(mRevealStampAnimation.mRevealEntity))
	{
		revealSprite->mVisible = true;
		revealSprite->mColorTint = Vec4(1.0f, 1.0f, 1.0f, 0.0f);
		revealSprite->SetVisibleRangeNormalizedX(0.0f, 0.0f);
		revealSprite->SetVisibleRangeKeepDestinationSize(true);
	}

	if (UISpriteComponent* stampSprite =
		mWorld->GetComponent<UISpriteComponent>(mRevealStampAnimation.mStampEntity))
	{
		stampSprite->mVisible = false;
		stampSprite->mColorTint = Vec4(1.0f, 1.0f, 1.0f, 0.0f);
	}

	if (UITransformComponent* stampTransform =
		mWorld->GetComponent<UITransformComponent>(mRevealStampAnimation.mStampEntity))
	{
		stampTransform->mScale =
			Vec2(spec.mStampStartScale, spec.mStampStartScale);
	}

	if (UISpriteComponent* finalStampSprite =
		mWorld->GetComponent<UISpriteComponent>(mRevealStampAnimation.mFinalStampEntity))
	{
		finalStampSprite->mVisible = false;
		finalStampSprite->mColorTint = Vec4(1.0f, 1.0f, 1.0f, 0.0f);
	}

	if (UITransformComponent* finalStampTransform =
		mWorld->GetComponent<UITransformComponent>(mRevealStampAnimation.mFinalStampEntity))
	{
		finalStampTransform->mScale =
			Vec2(spec.mFinalStampStartScale, spec.mFinalStampStartScale);
	}
}

void UIGameInfoUpdateFeature::StopRevealStampAnimation()
{
	mRevealStampAnimation.mActive = false;

	if (UISpriteComponent* revealSprite =
		mWorld->GetComponent<UISpriteComponent>(mRevealStampAnimation.mRevealEntity))
	{
		revealSprite->mVisible = false;
	}

	if (UISpriteComponent* stampSprite =
		mWorld->GetComponent<UISpriteComponent>(mRevealStampAnimation.mStampEntity))
	{
		stampSprite->mVisible = false;
	}

	if (UISpriteComponent* finalStampSprite =
		mWorld->GetComponent<UISpriteComponent>(mRevealStampAnimation.mFinalStampEntity))
	{
		finalStampSprite->mVisible = false;
	}
}

void UIGameInfoUpdateFeature::TriggerAnimationCameraShake(
	const RevealStampAnimationSpec& spec)
{
	if (!mWorld->HasComponentPool<CameraTypeComponent>())
		return;

	// Stamp가 나타나는 프레임에 설정값으로 현재 게임 카메라를 흔든다.
	for (Entity cameraEntity : mWorld->GetEntitiesWithComponent<CameraTypeComponent>())
	{
		if (CameraTypeComponent* camera = mWorld->GetComponent<CameraTypeComponent>(cameraEntity))
		{
			camera->TriggerShake(
				spec.mCameraShakeAngles,
				spec.mCameraShakeDuration,
				spec.mCameraShakeFrequency);
		}
	}
}

void UIGameInfoUpdateFeature::EnsurePhaseStatusText()
{
	if (mPhaseStatusTextEntity != NULL_ENTITY)
		return;

	mPhaseStatusTextEntity = mWorld->CreateEntity();

	auto& transform = mWorld->AddComponent<UITransformComponent>(mPhaseStatusTextEntity);
	transform.mAnchor = Anchor::Center;
	transform.mPivot = Vec2(0.5f, 0.5f);
	transform.mSize = Vec2(900.0f, 400.0f);
	transform.mPosition = Vec2(0.0f, 0.0f);
	transform.mUILayerIndex = 10;

	auto& text = mWorld->AddComponent<UITextComponent>(mPhaseStatusTextEntity);
	text.mFontType = UIFontType::Esamanru;
	text.mVisible = false;
	text.mOutlineThickness = 2.0f;
}

void UIGameInfoUpdateFeature::ApplyBannerSpec(const PhaseGoalSpec& spec)
{
	auto applyLayout = [&](Entity ent)
	{
		auto* tr = mWorld->GetComponent<UITransformComponent>(ent);
		if (tr == nullptr) return;

		if (spec.mFullscreen)	// 풀스크린
		{
			tr->mAnchor = Anchor::Center;
			tr->mPivot  = Vec2(0.5f, 0.5f);
			tr->UseScreenRatioLayout(Vec2(0.f, 0.f), Vec2(1.f, 1.f));
		}
		else
		{
			tr->mLayoutMode = UILayoutMode::ReferenceResolution;
			tr->mSize       = spec.mBannerSize;
		}
		tr->mScale = Vec2(1.f, 1.f); // 새 phase 진입 시 스케일 초기화.
	};

	applyLayout(mBannerEntity);
	applyLayout(mBannerTextEntity);

	if (auto* spr = mWorld->GetComponent<UISpriteComponent>(mBannerEntity))
	{
		spr->mTexture = RESOURCEMANAGER.Get<Texture>(spec.mTextureName);
	}

	if (auto* txt = mWorld->GetComponent<UITextComponent>(mBannerTextEntity))
	{
		txt->mText = spec.mTitleText;
	}
}

void UIGameInfoUpdateFeature::SetBannerVisible(bool visible)
{
	if (mBannerEntity != NULL_ENTITY)
	{
		if (auto* spr = mWorld->GetComponent<UISpriteComponent>(mBannerEntity))
			spr->mVisible = visible;
	}
}

void UIGameInfoUpdateFeature::SetPhaseStatusVisible(bool visible)
{
	if (mPhaseStatusTextEntity == NULL_ENTITY)
		return;

	if (UITextComponent* text = mWorld->GetComponent<UITextComponent>(mPhaseStatusTextEntity))
		text->mVisible = visible;
}

void UIGameInfoUpdateFeature::DrawDebugPanel()
{
#ifdef _IMGUI
	if (!ImGui::Begin("Goal Banner Debug"))
	{
		ImGui::End();
		return;
	}

	// 현재 상태머신 상태.
	const char* stageNames[] = { "Idle", "Intro", "Settle", "Hold", "Outro", "Done" };
	const int stageIdx = std::min((int)mStage, (int)(sizeof(stageNames) / sizeof(stageNames[0])) - 1);

	ImGui::Text("Current Phase : %d", (int)mCurrentPhase);
	ImGui::Text("Stage         : %s", stageNames[stageIdx]);
	ImGui::Text("Elapsed       : %.3f s", mStageElapsed);
	ImGui::Text("PulsePhase    : %.3f", mPulsePhase);
	ImGui::Separator();

	// phase 강제 트리거. mCurrentPhase 는 건드리지 않으므로
	// 게임 룰의 mGamePhase 가 그대로면 다음 프레임에 덮어쓰지 않는다.
	ImGui::Text("Force Trigger");
	struct PhaseBtn { const char* label; WavePhaseType phase; };
	const PhaseBtn buttons[] = {
		{ "Prepare",  WavePhaseType::Prepare  },
		{ "Conquest", WavePhaseType::Conquest },
		{ "Escort",   WavePhaseType::Escort   },
		{ "Boss",     WavePhaseType::Boss     },
		{ "Fail",     WavePhaseType::Fail     },
		{ "Clear",    WavePhaseType::Clear    },
	};
	for (int i = 0; i < (int)(sizeof(buttons) / sizeof(buttons[0])); ++i)
	{
		if (i % 3 != 0) ImGui::SameLine();
		if (ImGui::Button(buttons[i].label, ImVec2(90, 0)))
		{
			OnPhaseChanged(buttons[i].phase);
		}
	}

	ImGui::Separator();

	// 현재 활성 spec 의 인트로/정착/아웃트로 표시.
	const char* introNames[]  = { "None","FadeInCenter","PopScale","ZoomBurst",
	                              "SlideInLeft","SlideInRight","DropFromTop","Flash" };
	const char* settledNames[]= { "Static","MoveToTop","BeatPulse","Shake" };
	const char* outroNames[]  = { "None","FadeOut","SlideOutLeft","SlideOutRight","ScaleOut" };

	ImGui::Text("Active Spec");
	ImGui::Text("  Intro   : %s", introNames[(int)mActiveSpec.mIntro]);
	ImGui::Text("  Settled : %s", settledNames[(int)mActiveSpec.mSettled]);
	ImGui::Text("  Outro   : %s", outroNames[(int)mActiveSpec.mOutro]);

	ImGui::Separator();

	// 현재 phase 의 spec 파라미터 라이브 튜닝.
	auto it = mTable.find(mCurrentPhase);
	if (it != mTable.end())
	{
		UiAnimParams& p = it->second.mParams;
		if (ImGui::CollapsingHeader("Tweak Active Phase Params"))
		{
			ImGui::DragFloat("IntroDuration",  &p.mIntroDuration,  0.01f, 0.f, 5.f);
			ImGui::DragFloat("SettleDuration", &p.mSettleDuration, 0.01f, 0.f, 5.f);
			ImGui::DragFloat("HoldDuration",   &p.mHoldDuration,   0.01f, 0.f, 30.f);
			ImGui::DragFloat("OutroDuration",  &p.mOutroDuration,  0.01f, 0.f, 5.f);
			ImGui::DragFloat2("TopAnchorPos",  &p.mTopAnchorPos.x, 1.f);
			ImGui::DragFloat2("SlideOffset",   &p.mSlideOffset.x,  10.f);
			ImGui::DragFloat2("DropFromOffset",&p.mDropFromOffset.x, 10.f);
			ImGui::DragFloat("PulseAmplitude", &p.mPulseAmplitude, 0.01f, 0.f, 0.5f);
			ImGui::DragFloat("ShakeAmplitude", &p.mShakeAmplitude, 0.1f,  0.f, 50.f);
			ImGui::DragFloat("ShakeFrequency", &p.mShakeFrequency, 0.5f,  0.f, 80.f);
			ImGui::DragFloat("OvershootScale", &p.mOvershootScale, 0.01f, 1.f, 2.f);
			ImGui::DragFloat("ZoomBurstStart", &p.mZoomBurstStart, 0.05f, 1.f, 5.f);
		}
	}

	ImGui::End();
#endif
}
