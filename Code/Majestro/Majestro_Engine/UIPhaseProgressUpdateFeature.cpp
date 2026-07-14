#include "pch.h"
#include "UIPhaseProgressUpdateFeature.h"
#include "MathUtils.h"
#include "GameRuleComponent.h"
#include "GameMode.h"
#include "GameEvents.h"
#include "EventManager.h"

#include "Engine.h"
#include "RenderManager.h"
#include "ResourceManager.h"
#include "Mesh.h"
#include "Shader.h"
#include "Texture.h"
#include "World.h"
#include "UIHpBarUpdateFeature.h"

namespace
{
	// 살짝 튀어 올랐다 정착하는 ease-out-back (포커스 pop 연출용)
	float EaseOutBack(float t)
	{
		t = std::clamp(t, 0.f, 1.f);
		constexpr float c1 = 1.70158f;
		constexpr float c3 = c1 + 1.f;
		const float p = t - 1.f;
		return 1.f + c3 * p * p * p + c1 * p * p;
	}
}

void UIPhaseProgressUpdateFeature::Initialize(World* world)
{
	UIFeature::Initialize(world);
}

void UIPhaseProgressUpdateFeature::Update(float dt)
{
	Entity e = mWorld->GetSingletonEntity();

	GameRuleComponent* gameRuleComp = mWorld->GetComponent<GameRuleComponent>(e);

	// std::cout << "UIPhaseProgressUpdateFeature::Update - Game Phase: " << static_cast<int>(gameRuleComp->mGamePhase) << std::endl;
	switch (gameRuleComp->mGamePhase)
	{
		case uint8(WavePhaseType::Prepare): // Prepare
			break;
		case uint8(WavePhaseType::Conquest): { // Conquest

			GameConquestComponent* gameConquestComp = mWorld->GetComponent<GameConquestComponent>(e);
			if (gameConquestComp) UpdateConquestProgress(dt, gameConquestComp);
			break;
		}
		case uint8(WavePhaseType::Escort): { // Escort
			GameEscortComponent* gameEscortComp = mWorld->GetComponent<GameEscortComponent>(e);
			if (gameEscortComp) UpdateEscortProgress(dt, gameEscortComp);
			break;
		}
		default:
			break;
	}
}

void UIPhaseProgressUpdateFeature::WorldRender(CameraComponent* camera)
{
}

void UIPhaseProgressUpdateFeature::SpriteRender(DirectX::SpriteBatch* spriteBatch)
{
}

void UIPhaseProgressUpdateFeature::CustomSpriteRender(std::vector<UIInstanceData>& instances)
{
}

void UIPhaseProgressUpdateFeature::PostSpriteRender(std::vector<UIInstanceData>& /*instances*/)
{


	Entity e = mWorld->GetSingletonEntity();

	GameRuleComponent* gameRuleComp = mWorld->GetComponent<GameRuleComponent>(e);

	switch (gameRuleComp->mGamePhase)
	{
	case uint8(WavePhaseType::Prepare): // Prepare

		break;
	case uint8(WavePhaseType::Conquest): { // Conquest

		GameConquestComponent* gameConquestComp = mWorld->GetComponent<GameConquestComponent>(e);
		if (gameConquestComp)
		{
			// SecondGame 만 3개 점령지 표시. 그 외(FirstGame 등)는 기존 단일 링 유지.
			if (mWorld->GetSceneId() == SceneId::SecondGame)
				DrawConquestRingMulti();
			else
				DrawConquestRing();
		}
		break;
	}
	case uint8(WavePhaseType::Escort): { // Escort

		GameEscortComponent* gameEscortComp = mWorld->GetComponent<GameEscortComponent>(e);
		if (gameEscortComp) DrawEscortBar();
		break;
	}
	case uint8(WavePhaseType::Boss): {// Boss
		break;
	}
	default:

		break;
	}

#ifdef _IMGUI
	DrawDebugPanel();
#endif

}

// Phase Update

void UIPhaseProgressUpdateFeature::UpdateConquestProgress(float dt, GameConquestComponent* conquestComp)
{
	const float total = conquestComp->mRequiredConquestTime;
	const float ratio = (total > 0.f) ? (conquestComp->mWaveTime / total) : 0.f;
	mConquestProgress       = std::clamp(ratio, 0.f, 1.f);
	mCachedConquestWaveCheckPoint = conquestComp->mWaveCheckPoint;

	// SecondGame 전용: 3개 점령지 포커스 이동 + 완료 연출 (FirstGame 등은 손대지 않음)
	if (mWorld->GetSceneId() != SceneId::SecondGame)
		return;

	const int32 wave = std::clamp(conquestComp->mWave, 1, GameConquestComponent::mMaxConquest);

	// 점령지 완료 = 웨이브 번호 증가 감지
	if (wave > mPrevWave)
	{
		mCompletedRingIdx = mPrevWave - 1; // 방금 채워진 링 (0-based)
		mFocusAnimT       = 0.f;           // 포커스 이동/플래시 시작

	}
	mPrevWave = wave;

	// 포커스 이동/플래시 애니메이션 진행
	if (mFocusAnimT < 1.f)
	{
		const float step = (mFocusAnimDuration > 0.f) ? (dt / mFocusAnimDuration) : 1.f;
		mFocusAnimT = std::min(1.f, mFocusAnimT + step);
		if (mFocusAnimT >= 1.f)
			mCompletedRingIdx = -1; // 플래시 종료
	}
}

void UIPhaseProgressUpdateFeature::UpdateEscortProgress(float dt, GameEscortComponent* escortComp)
{
	if (escortComp->mStageCount == 0)	// 서버가 구간 정보를 아직 안 보냄
	{
		mEscortProgress = std::clamp(escortComp->mEscortProgress, 0.f, 1.f);
		return;
	}

	const int32 stageCount = static_cast<int32>(escortComp->mStageCount);
	const int32 stage      = std::clamp<int32>(escortComp->mEscortStage, 0, stageCount - 1);
	const float stageLocal = std::clamp(escortComp->mStageProgress, 0.f, 1.f);

	// displayedFill = (현재 stage + 구간 내 진행도) / 전체 구간 수
	mEscortProgress = std::clamp((stage + stageLocal) / static_cast<float>(stageCount), 0.f, 1.f);
}

// Draw

Vec2 UIPhaseProgressUpdateFeature::GetProgressAnchorPx() const
{
	const WindowInfo& window = RENDERMANAGER.GetWindow();

	return Vec2(
		static_cast<float>(window.Width) * std::clamp(mProgressAnchorRatio.x, 0.f, 1.f),
		static_cast<float>(window.Height) * std::clamp(mProgressAnchorRatio.y, 0.f, 1.f));
}

Vec2 UIPhaseProgressUpdateFeature::GetProgressSizePx(const Vec2& sizeRatio) const
{
	const WindowInfo& window = RENDERMANAGER.GetWindow();

	return Vec2(
		static_cast<float>(window.Width) * std::clamp(sizeRatio.x, 0.f, 1.f),
		static_cast<float>(window.Height) * std::clamp(sizeRatio.y, 0.f, 1.f));
}

void UIPhaseProgressUpdateFeature::DrawConquestRing()
{
	auto ringShader = RESOURCEMANAGER.Get<Shader>(L"WorldUIConquestRing");
	auto quadMesh   = RESOURCEMANAGER.Get<Mesh>(L"UIQuad");
	if (ringShader == nullptr || quadMesh == nullptr)
		return;

	shared_ptr<Texture> bgTex   = RESOURCEMANAGER.Get<Texture>(mConquestBgTextureName);
	shared_ptr<Texture> fillTex = RESOURCEMANAGER.Get<Texture>(mConquestFillTextureName);
	if (bgTex == nullptr)
		return;
	if (fillTex == nullptr)
		fillTex = bgTex;


	int8 backIndex = RENDERMANAGER.GetSwapChain()->GetBackBufferIndex();
	RENDERMANAGER.GetRenderTargetGroup(static_cast<uint32>(RENDER_TARGET_GROUP_TYPE::SWAP_CHAIN))
		.OMSetRenderTargets(1, backIndex);

	RENDERMANAGER.SetGraphicsTable();


	GlobalParamsLayout gp{};
	gp.BaseInstanceID = 0;
	gp.PassScalar0            = 1; // HUD 모드
	// PassScalar1 에 innerRadius * 1000 을 정수로 인코딩 (PS 에서 / 1000.0 으로 복원)
	gp.PassScalar1        = static_cast<uint32>(std::clamp(mConquestInnerRadius, 0.f, 1.f) * 1000.f);
	gp.PassCustomIndex = 0;

	// 앵커 = 화면 픽셀, 피벗으로 좌상단을 (-w/2, -h/2) 만큼 옮겨 앵커가 정사각형 중심
	const Vec2 anchorPx = GetProgressAnchorPx();
	const Vec2 conquestSizePx = GetProgressSizePx(mConquestSizeRatio);
	// 창 너비가 바뀌어도 중앙 정렬이 유지되도록 계산된 앵커 사용
	gp.HpBarAnchorWorldX = anchorPx.x;
	gp.HpBarAnchorWorldY = anchorPx.y;
	gp.HpBarAnchorWorldZ = 0.f;
	gp.HpBarFollowRatio  = std::clamp(mConquestProgress, 0.f, 1.f);

	gp.HpBarSizePxX = conquestSizePx.x;
	gp.HpBarSizePxY = conquestSizePx.y;
	gp.HpBarPivotPxX = -conquestSizePx.x * 0.5f;
	gp.HpBarPivotPxY = -conquestSizePx.y * 0.5f;

	gp.HpBarBgTexIdx   = bgTex->GetImageIndex();
	gp.HpBarFillTexIdx = fillTex->GetImageIndex();
	gp.HpBarHitTexIdx  = 0;
	gp.HpBarHitConfig  = 0;

	GRAPHICS_CMD_LIST->SetGraphicsRoot32BitConstants(0, 16, &gp, 0);

	ringShader->Update();
	quadMesh->Render(1, 0, 0, 0);

	const uint32 zero = 0;
	GRAPHICS_CMD_LIST->SetGraphicsRoot32BitConstants(0, 1, &zero, 0);
	GRAPHICS_CMD_LIST->SetGraphicsRoot32BitConstants(0, 1, &zero, 2);
}

void UIPhaseProgressUpdateFeature::DrawConquestRingMulti()
{
	auto ringShader = RESOURCEMANAGER.Get<Shader>(L"WorldUIConquestRing");
	auto quadMesh   = RESOURCEMANAGER.Get<Mesh>(L"UIQuad");
	if (ringShader == nullptr || quadMesh == nullptr)
		return;

	shared_ptr<Texture> bgTex   = RESOURCEMANAGER.Get<Texture>(mConquestBgTextureName);
	shared_ptr<Texture> fillTex = RESOURCEMANAGER.Get<Texture>(mConquestFillTextureName);
	if (bgTex == nullptr)
		return;
	if (fillTex == nullptr)
		fillTex = bgTex;

	int8 backIndex = RENDERMANAGER.GetSwapChain()->GetBackBufferIndex();
	RENDERMANAGER.GetRenderTargetGroup(static_cast<uint32>(RENDER_TARGET_GROUP_TYPE::SWAP_CHAIN))
		.OMSetRenderTargets(1, backIndex);

	RENDERMANAGER.SetGraphicsTable();

	const WindowInfo& window = RENDERMANAGER.GetWindow();
	const Vec2 anchorPx   = GetProgressAnchorPx();
	const Vec2 baseSizePx = GetProgressSizePx(mConquestSizeRatio);
	const float gapPx     = static_cast<float>(window.Width) * std::clamp(mConquestRingGapRatio, 0.f, 1.f);

	const int32 maxConquest = GameConquestComponent::mMaxConquest;
	const int32 current     = std::clamp(mPrevWave - 1, 0, maxConquest - 1);
	const float t           = std::clamp(mFocusAnimT, 0.f, 1.f);

	const uint32 innerEnc = static_cast<uint32>(std::clamp(mConquestInnerRadius, 0.f, 1.f) * 1000.f);

	// 링 1개 그리기. flash01: 1=기본 불투명, 0..1=완료 플래시 알파.
	auto drawRing = [&](float centerX, const Vec2& sizePx, float progress, float flash01)
	{
		GlobalParamsLayout gp{};
		gp.BaseInstanceID  = 0;
		gp.PassScalar0             = 1; // HUD 모드
		gp.PassScalar1         = innerEnc;
		// PassCustomIndex: 0=기본(불투명). 1..1000 = 완료 플래시 알파*1000 (PS 에서 복원)
		gp.PassCustomIndex = (flash01 >= 0.999f) ? 0u
		                   : static_cast<uint32>(std::clamp(flash01, 0.f, 1.f) * 1000.f);

		gp.HpBarAnchorWorldX = centerX;
		gp.HpBarAnchorWorldY = anchorPx.y;
		gp.HpBarAnchorWorldZ = 0.f;
		gp.HpBarFollowRatio  = std::clamp(progress, 0.f, 1.f);

		gp.HpBarSizePxX  = sizePx.x;
		gp.HpBarSizePxY  = sizePx.y;
		gp.HpBarPivotPxX = -sizePx.x * 0.5f;
		gp.HpBarPivotPxY = -sizePx.y * 0.5f;

		gp.HpBarBgTexIdx   = bgTex->GetImageIndex();
		gp.HpBarFillTexIdx = fillTex->GetImageIndex();
		gp.HpBarHitTexIdx  = 0;
		gp.HpBarHitConfig  = 0;

		GRAPHICS_CMD_LIST->SetGraphicsRoot32BitConstants(0, 16, &gp, 0);
		ringShader->Update();
		quadMesh->Render(1, 0, 0, 0);
	};

	for (int32 i = 0; i < maxConquest; ++i)
	{
		// 앵커(중앙) 기준 좌우 대칭 배치
		const float centerX = anchorPx.x + (static_cast<float>(i) - (maxConquest - 1) * 0.5f) * gapPx;

		float progress;
		if      (i <  current) progress = 1.f;               // 완료된 점령지
		else if (i == current) progress = mConquestProgress; // 진행중 점령지
		else                   progress = 0.f;               // 대기중 점령지

		// 포커스 이동: 현재 링은 (대기/완료 배율 -> 활성 배율)로 pop, 직전 링은 반대로 빠짐
		float scale = mConquestInactiveScale;
		const float pop = ::EaseOutBack(t);
		if (i == current)
			scale = mConquestInactiveScale + (mConquestActiveScale - mConquestInactiveScale) * pop;
		else if (i == current - 1 && t < 1.f)
			scale = mConquestInactiveScale + (mConquestActiveScale - mConquestInactiveScale) * (1.f - t);

		drawRing(centerX, baseSizePx * scale, progress, 1.f);
	}

	// 완료 플래시: 방금 채워진 링 위에 확대 + 페이드 오버레이
	if (mCompletedRingIdx >= 0 && mCompletedRingIdx < maxConquest && t < 1.f)
	{
		const int32 i = mCompletedRingIdx;
		const float centerX    = anchorPx.x + (static_cast<float>(i) - (maxConquest - 1) * 0.5f) * gapPx;
		const float flashScale = mConquestActiveScale + 0.6f * t; // 점점 확대
		const float flashAlpha = 1.f - t;                          // 점점 투명
		drawRing(centerX, baseSizePx * flashScale, 1.f, flashAlpha);
	}

	// 후속 패스 보호 — BaseInstanceID / PassScalar1 / PassCustomIndex 원복
	const uint32 zero = 0;
	GRAPHICS_CMD_LIST->SetGraphicsRoot32BitConstants(0, 1, &zero, 0);
	GRAPHICS_CMD_LIST->SetGraphicsRoot32BitConstants(0, 1, &zero, 2);
	GRAPHICS_CMD_LIST->SetGraphicsRoot32BitConstants(0, 1, &zero, 3);
}

void UIPhaseProgressUpdateFeature::DrawDebugPanel()
{
#ifdef _IMGUI
	if (!ImGui::Begin("Phase Progress Debug"))
	{
		ImGui::End();
		return;
	}

	const WindowInfo& window = RENDERMANAGER.GetWindow();
	const Vec2 anchorPx = GetProgressAnchorPx();
	const Vec2 conquestSizePx = GetProgressSizePx(mConquestSizeRatio);
	const Vec2 escortSizePx = GetProgressSizePx(mEscortSizeRatio);
	const Vec2 escortCursorSizePx = GetProgressSizePx(mEscortCursorSizeRatio);

	ImGui::Text("Window : %d x %d", window.Width, window.Height);
	ImGui::Text("Anchor : %.1f, %.1f", anchorPx.x, anchorPx.y);
	ImGui::Separator();

	// X와 Y는 각각 화면 너비와 높이 기준 비율
	ImGui::DragFloat2("AnchorRatio", &mProgressAnchorRatio.x, 0.001f, 0.f, 1.f, "%.4f");
	mProgressAnchorRatio.x = std::clamp(mProgressAnchorRatio.x, 0.f, 1.f);
	mProgressAnchorRatio.y = std::clamp(mProgressAnchorRatio.y, 0.f, 1.f);

	if (ImGui::Button("Reset Anchor Ratio"))
	{
		mProgressAnchorRatio = Vec2(0.5f, 0.1019f);
	}

	ImGui::Separator();

	// ImGui에서 수정한 값이 셰이더에 들어가기 전에 유효 범위로 보정
	if (ImGui::CollapsingHeader("Conquest", ImGuiTreeNodeFlags_DefaultOpen))
	{
		ImGui::DragFloat2("ConquestSizeRatio", &mConquestSizeRatio.x, 0.001f, 0.f, 1.f, "%.4f");
		ImGui::Text("ConquestSizePx : %.1f, %.1f", conquestSizePx.x, conquestSizePx.y);
		ImGui::SliderFloat("ConquestInnerRadius", &mConquestInnerRadius, 0.f, 1.f);


		mConquestSizeRatio.x = std::clamp(mConquestSizeRatio.x, 0.f, 1.f);
		mConquestSizeRatio.y = std::clamp(mConquestSizeRatio.y, 0.f, 1.f);
		mConquestInnerRadius = std::clamp(mConquestInnerRadius, 0.f, 1.f);

		// SecondGame 3개 점령지 표시 전용 튜닝값
		ImGui::Separator();
		ImGui::Text("Conquest Multi (SecondGame)");
		ImGui::DragFloat("RingGapRatio", &mConquestRingGapRatio, 0.001f, 0.f, 0.5f, "%.4f");
		ImGui::DragFloat("ActiveScale", &mConquestActiveScale, 0.01f, 0.5f, 3.f, "%.2f");
		ImGui::DragFloat("InactiveScale", &mConquestInactiveScale, 0.01f, 0.3f, 1.5f, "%.2f");
		ImGui::DragFloat("FocusAnimDuration", &mFocusAnimDuration, 0.01f, 0.f, 2.f, "%.2f");
		ImGui::Text("PrevWave : %d / %d  (focusT %.2f)", mPrevWave, GameConquestComponent::mMaxConquest, mFocusAnimT);
		if (ImGui::Button("Test Focus Anim"))
		{
			mCompletedRingIdx = std::clamp(mPrevWave - 1, 0, GameConquestComponent::mMaxConquest - 1);
			mFocusAnimT = 0.f;
		}

		mConquestRingGapRatio  = std::clamp(mConquestRingGapRatio, 0.f, 0.5f);
		mConquestActiveScale   = std::clamp(mConquestActiveScale, 0.5f, 3.f);
		mConquestInactiveScale = std::clamp(mConquestInactiveScale, 0.3f, 1.5f);
		mFocusAnimDuration     = std::clamp(mFocusAnimDuration, 0.f, 2.f);
	}

	if (ImGui::CollapsingHeader("Escort", ImGuiTreeNodeFlags_DefaultOpen))
	{
		ImGui::DragFloat2("EscortSizeRatio", &mEscortSizeRatio.x, 0.001f, 0.f, 1.f, "%.4f");
		ImGui::Text("EscortSizePx : %.1f, %.1f", escortSizePx.x, escortSizePx.y);
		ImGui::DragFloat2("EscortCursorSizeRatio", &mEscortCursorSizeRatio.x, 0.001f, 0.f, 1.f, "%.4f");
		ImGui::Text("EscortCursorSizePx : %.1f, %.1f", escortCursorSizePx.x, escortCursorSizePx.y);

		// 텍스처 캔버스 내 실제 트랙 UV 구간 (x=시작U, y=끝U)
		ImGui::DragFloat2("EscortFillUvRange", &mEscortFillUvRange.x, 0.001f, 0.f, 1.f, "%.4f");

		mEscortSizeRatio.x = std::clamp(mEscortSizeRatio.x, 0.f, 1.f);
		mEscortSizeRatio.y = std::clamp(mEscortSizeRatio.y, 0.f, 1.f);
		mEscortCursorSizeRatio.x = std::clamp(mEscortCursorSizeRatio.x, 0.f, 1.f);
		mEscortCursorSizeRatio.y = std::clamp(mEscortCursorSizeRatio.y, 0.f, 1.f);
		mEscortFillUvRange.x = std::clamp(mEscortFillUvRange.x, 0.f, 1.f);
		mEscortFillUvRange.y = std::clamp(mEscortFillUvRange.y, mEscortFillUvRange.x, 1.f);
	}

	ImGui::End();
#endif
}

void UIPhaseProgressUpdateFeature::DrawEscortBar()
{
	auto spriteShader = RESOURCEMANAGER.Get<Shader>(L"WorldUIHpSprite");
	auto quadMesh     = RESOURCEMANAGER.Get<Mesh>(L"UIQuad");
	if (spriteShader == nullptr || quadMesh == nullptr)
		return;

	shared_ptr<Texture> bgTex     = RESOURCEMANAGER.Get<Texture>(mEscortBgTextureName);
	shared_ptr<Texture> lineTex   = RESOURCEMANAGER.Get<Texture>(mEscortLineTextureName);
	shared_ptr<Texture> checkTex  = RESOURCEMANAGER.Get<Texture>(mEscortCheckTextureName);
	shared_ptr<Texture> cursorTex = RESOURCEMANAGER.Get<Texture>(mEscortCursorTextureName);

	// 라인이 없으면 바 자체를 그릴 수 없음 (커서/BG 도 의미 없음)
	if (lineTex == nullptr)
		return;
	if (checkTex == nullptr)
		checkTex = lineTex;

	int8 backIndex = RENDERMANAGER.GetSwapChain()->GetBackBufferIndex();
	RENDERMANAGER.GetRenderTargetGroup(static_cast<uint32>(RENDER_TARGET_GROUP_TYPE::SWAP_CHAIN))
		.OMSetRenderTargets(1, backIndex);

	RENDERMANAGER.SetGraphicsTable();

	const float fillRatio = std::clamp(mEscortProgress, 0.f, 1.f);
	// 텍스처 여백 보정
	const float fillCutU = RemapBarRatioToUv(fillRatio, mEscortFillUvRange);
	const Vec2 escortSizePx = GetProgressSizePx(mEscortSizeRatio);
	const Vec2 escortCursorSizePx = GetProgressSizePx(mEscortCursorSizeRatio);

	// 공통 GlobalParams (HUD 모드, 앵커는 바 정중앙)
	GlobalParamsLayout gp{};
	gp.BaseInstanceID    = 0;
	gp.PassScalar0               = 1; // HUD 모드 (스크린 스페이스, depth occlusion 스킵)
	gp.PassCustomIndex   = 0;
	const Vec2 anchorPx = GetProgressAnchorPx();
	
	gp.HpBarAnchorWorldX = anchorPx.x;
	gp.HpBarAnchorWorldY = anchorPx.y;
	gp.HpBarAnchorWorldZ = 0.f;
	gp.HpBarSizePxX      = escortSizePx.x;
	gp.HpBarSizePxY      = escortSizePx.y;
	gp.HpBarPivotPxX     = -escortSizePx.x * 0.5f;
	gp.HpBarPivotPxY     = -escortSizePx.y * 0.5f;
	gp.HpBarFollowRatio  = fillCutU;
	gp.HpBarHitTexIdx    = 0;
	gp.HpBarHitConfig    = 0;

	// 1) BG (가장 뒤, 풀 출력) — role=0 → BgTexIdx 사용
	if (bgTex != nullptr)
	{
		gp.PassScalar1         = 0;
		gp.HpBarBgTexIdx   = bgTex->GetImageIndex();
		gp.HpBarFillTexIdx = bgTex->GetImageIndex();
		GRAPHICS_CMD_LIST->SetGraphicsRoot32BitConstants(0, 16, &gp, 0);
		spriteShader->Update();
		quadMesh->Render(1, 0, 0, 0);
	}

	// 2) Line (빈 트랙) + 3) Check (채움) — 한 번의 GlobalParams 셋업으로 두 패스
	gp.PassScalar1         = 0;
	gp.HpBarBgTexIdx   = lineTex->GetImageIndex();
	gp.HpBarFillTexIdx = checkTex->GetImageIndex();
	GRAPHICS_CMD_LIST->SetGraphicsRoot32BitConstants(0, 16, &gp, 0);
	spriteShader->Update();
	quadMesh->Render(1, 0, 0, 0);                     // line (role=0)

	const uint32 fillRole = 1;
	GRAPHICS_CMD_LIST->SetGraphicsRoot32BitConstants(0, 1, &fillRole, 2);
	quadMesh->Render(1, 0, 0, 0);                     // check (role=1, uv.x>fillCutU discard)

	// 4) Cursor — 같은 앵커, Pivot 만 진행도 위치로 평행 이동
	if (cursorTex != nullptr)
	{
		gp.PassScalar1         = 0;
		gp.HpBarSizePxX    = escortCursorSizePx.x;
		gp.HpBarSizePxY    = escortCursorSizePx.y;
		gp.HpBarPivotPxX   = -escortSizePx.x * 0.5f
		                   + fillCutU * escortSizePx.x
		                   - escortCursorSizePx.x * 0.5f;
		// 세로: 바와 같은 중앙선 정렬
		gp.HpBarPivotPxY   = -escortCursorSizePx.y * 0.5f;
		gp.HpBarBgTexIdx   = cursorTex->GetImageIndex();
		gp.HpBarFillTexIdx = cursorTex->GetImageIndex();
		GRAPHICS_CMD_LIST->SetGraphicsRoot32BitConstants(0, 16, &gp, 0);
		spriteShader->Update();
		quadMesh->Render(1, 0, 0, 0);
	}

	// 후속 패스 보호 — DrawConquestRing 과 동일하게 BaseInstanceID / PassScalar1 원복
	const uint32 zero = 0;
	GRAPHICS_CMD_LIST->SetGraphicsRoot32BitConstants(0, 1, &zero, 0);
	GRAPHICS_CMD_LIST->SetGraphicsRoot32BitConstants(0, 1, &zero, 2);
}
