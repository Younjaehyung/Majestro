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

namespace
{
	struct UIPhaseSpriteParamsLayout
	{
		uint32 BaseInstanceID = 0;
		uint32 PassFlags = 0;
		uint32 SpriteRole = 0;
		uint32 ReservedHeader = 0;

		float AnchorX = 0.f;
		float AnchorY = 0.f;
		float AnchorZ = 0.f;
		float Progress = 1.f;

		float SizePxX = 0.f;
		float SizePxY = 0.f;
		float PivotPxX = 0.f;
		float PivotPxY = 0.f;

		uint32 BackgroundTextureIndex = 0;
		uint32 FillTextureIndex = 0;
		uint32 Reserved0 = 0;
		uint32 Reserved1 = 0;
	};

	// 점령 링 셰이더
	struct UIConquestRingParamsLayout
	{
		uint32 BaseInstanceID = 0;
		uint32 PassFlags = 0;
		uint32 InnerRadiusEncoded = 0;
		uint32 AlphaEncoded = 0;

		float AnchorX = 0.f;
		float AnchorY = 0.f;
		float AnchorZ = 0.f;
		float Progress = 0.f;

		float SizePxX = 0.f;
		float SizePxY = 0.f;
		float PivotPxX = 0.f;
		float PivotPxY = 0.f;

		uint32 BackgroundTextureIndex = 0;
		uint32 FillTextureIndex = 0;
		uint32 Reserved0 = 0;
		uint32 Reserved1 = 0;
	};

	static_assert(sizeof(UIPhaseSpriteParamsLayout) == 16 * 4, "UIPhaseSpriteParamsLayout must be 16 DWORDs");
	static_assert(sizeof(UIConquestRingParamsLayout) == 16 * 4, "UIConquestRingParamsLayout must be 16 DWORDs");

	uint32 EncodeAlpha(float alpha01)
	{
		return static_cast<uint32>(std::clamp(alpha01, 0.f, 1.f) * 1000.f);
	}

	constexpr float kBackRingCenterU  = 351.46f / 768.f;  //  0.4576
	constexpr float kBackRingCenterV  = -60.59f / 256.f;  // -0.2367 (원 중심이 텍스처 위쪽 바깥)
	constexpr float kBackRingRadiusU  = 148.46f / 768.f;  //  0.1933
	constexpr float kBackRingRadiusV  = 148.46f / 256.f;  //  0.5799
}

void UIPhaseProgressUpdateFeature::Initialize(World* world)
{
	UIFeature::Initialize(world);
}

void UIPhaseProgressUpdateFeature::Update(float dt)
{
	Entity e = mWorld->GetSingletonEntity();

	GameRuleComponent* gameRuleComp = mWorld->GetComponent<GameRuleComponent>(e);
	if (gameRuleComp == nullptr)
		return;

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

void UIPhaseProgressUpdateFeature::PostSpriteRender(std::vector<UIInstanceData>& /*instances*/)
{
	Entity e = mWorld->GetSingletonEntity();

	GameRuleComponent* gameRuleComp = mWorld->GetComponent<GameRuleComponent>(e);
	if (gameRuleComp == nullptr)
		return;

	switch (gameRuleComp->mGamePhase)
	{
	case uint8(WavePhaseType::Prepare): // Prepare

		break;
	case uint8(WavePhaseType::Conquest): { // Conquest

		GameConquestComponent* gameConquestComp = mWorld->GetComponent<GameConquestComponent>(e);
		if (gameConquestComp)
		{
			DrawConquestBackground();
			DrawConquestRoulette();
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
	mConquestProgress = std::clamp(ratio, 0.f, 1.f);

	mConquestZoneCount = std::max(1, mConquestZoneCount);

	if (!mDebugDriveRoulette)
	{
		const int32 zoneIdx = (conquestComp->mActiveZoneId > 0)
			? std::clamp<int32>(conquestComp->mActiveZoneId - 1, 0, mConquestZoneCount - 1)
			: std::clamp<int32>(conquestComp->mWave - 1, 0, mConquestZoneCount - 1);

		// 점령지가 넘어감 = 룰렛 한 칸 회전 + 직전 슬롯 완료 플래시
		if (zoneIdx > mCurrentZoneIdx)
		{
			mCompletedSlotIdx = mCurrentZoneIdx;
			mRollT            = 0.f;
			mCurrentZoneIdx   = zoneIdx;
		}
		else if (zoneIdx < mCurrentZoneIdx)
		{
			mCurrentZoneIdx   = zoneIdx;
			mCompletedSlotIdx = -1;
			mRollT            = 1.f;
		}
	}

	// 회전 애니메이션 진행
	if (mRollT < 1.f)
	{
		const float step = (mRollDuration > 0.f) ? (dt / mRollDuration) : 1.f;
		mRollT = std::min(1.f, mRollT + step);
		if (mRollT >= 1.f)
			mCompletedSlotIdx = -1; // 플래시 종료
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


void UIPhaseProgressUpdateFeature::DrawConquestBackground()
{
	shared_ptr<Texture> backTex = RESOURCEMANAGER.Get<Texture>(mConquestBackTextureName);
	if (backTex == nullptr)
		return;

	auto spriteShader = RESOURCEMANAGER.Get<Shader>(L"WorldUIHpSprite");
	auto quadMesh     = RESOURCEMANAGER.Get<Mesh>(L"UIQuad");

	if (spriteShader == nullptr || quadMesh == nullptr)
		return;

	int8 backIndex = RENDERMANAGER.GetSwapChain()->GetBackBufferIndex();
	RENDERMANAGER.GetRenderTargetGroup(static_cast<uint32>(RENDER_TARGET_GROUP_TYPE::SWAP_CHAIN))
		.OMSetRenderTargets(1, backIndex);

	RENDERMANAGER.SetGraphicsTable();

	const Vec2 anchorPx = GetConquestBackAnchorPx();
	const Vec2 sizePx   = GetProgressSizePx(mConquestBackSizeRatio);

	UIPhaseSpriteParamsLayout gp{};
	gp.BaseInstanceID         = 0;
	gp.PassFlags              = 1;
	gp.SpriteRole             = 0;
	gp.ReservedHeader         = 0;
	gp.AnchorX                = anchorPx.x;
	gp.AnchorY                = anchorPx.y;
	gp.AnchorZ                = 0.f;
	gp.Progress               = 1.f;
	gp.SizePxX                = sizePx.x;
	gp.SizePxY                = sizePx.y;
	gp.PivotPxX               = -sizePx.x * 0.5f;
	gp.PivotPxY               = -sizePx.y * 0.5f;
	gp.BackgroundTextureIndex = backTex->GetImageIndex();
	gp.FillTextureIndex       = backTex->GetImageIndex();

	GRAPHICS_CMD_LIST->SetGraphicsRoot32BitConstants(0, 16, &gp, 0);
	spriteShader->Update();
	quadMesh->Render(1, 0, 0, 0);
}

Vec2 UIPhaseProgressUpdateFeature::GetConquestBackAnchorPx() const
{
	const WindowInfo& window = RENDERMANAGER.GetWindow();

	return Vec2(
		static_cast<float>(window.Width)  * std::clamp(mConquestBackAnchorRatio.x, 0.f, 1.f),
		static_cast<float>(window.Height) * std::clamp(mConquestBackAnchorRatio.y, 0.f, 1.f));
}


void UIPhaseProgressUpdateFeature::GetConquestArcGeometry(Vec2& outCenterPx, Vec2& outRadiusPx) const
{
	const Vec2 backAnchorPx = GetConquestBackAnchorPx();
	const Vec2 backSizePx   = GetProgressSizePx(mConquestBackSizeRatio);


	outCenterPx = Vec2(
		backAnchorPx.x + (kBackRingCenterU - 0.5f) * backSizePx.x,
		backAnchorPx.y + (kBackRingCenterV - 0.5f) * backSizePx.y);

	outRadiusPx = Vec2(
		kBackRingRadiusU * backSizePx.x,
		kBackRingRadiusV * backSizePx.y);
}


Vec2 UIPhaseProgressUpdateFeature::GetConquestSlotPosPx(float slotOffset) const
{
	Vec2 centerPx, radiusPx;
	GetConquestArcGeometry(centerPx, radiusPx);

	constexpr float kDegToRad = 3.14159265f / 180.f;
	// 아래쪽 반원만 쓰도록 각도를 ±90도로 제한 (넘어가면 위쪽 반원으로 감겨 올라간다)
	const float theta = std::clamp(mConquestArcStepDegrees * slotOffset, -90.f, 90.f) * kDegToRad;

	return Vec2(
		centerPx.x + radiusPx.x * std::sin(theta),
		centerPx.y + radiusPx.y * std::cos(theta));
}


float UIPhaseProgressUpdateFeature::GetConquestSlotScale(float slotOffset) const
{
	const float w = MathUtils::SmoothStep01(1.f - std::min(std::abs(slotOffset), 1.f));
	return mConquestSideScale + (1.f - mConquestSideScale) * w;
}


float UIPhaseProgressUpdateFeature::GetConquestSlotBaseDiaPx() const
{
	Vec2 centerPx, radiusPx;
	GetConquestArcGeometry(centerPx, radiusPx);

	const float radiusAvgPx = (radiusPx.x + radiusPx.y) * 0.5f;
	return radiusAvgPx * std::max(mConquestSlotDiaOverRadius, 0.01f);
}

void UIPhaseProgressUpdateFeature::DrawConquestSlot(float slotOffset, float progress, float alpha, bool glow, float extraScale)
{
	auto ringShader = RESOURCEMANAGER.Get<Shader>(L"WorldUIConquestRing");
	auto quadMesh   = RESOURCEMANAGER.Get<Mesh>(L"UIQuad");
	shared_ptr<Texture> bgTex   = RESOURCEMANAGER.Get<Texture>(mConquestBgTextureName);
	shared_ptr<Texture> fillTex = RESOURCEMANAGER.Get<Texture>(mConquestFillTextureName);
	if (ringShader == nullptr || quadMesh == nullptr || bgTex == nullptr)
		return;
	if (fillTex == nullptr)
		fillTex = bgTex;

	const Vec2 posPx = GetConquestSlotPosPx(slotOffset);
	const float diaPx = GetConquestSlotBaseDiaPx() * GetConquestSlotScale(slotOffset) * extraScale;
	const Vec2  sizePx = Vec2(diaPx, diaPx);

	UIConquestRingParamsLayout gp{};
	gp.BaseInstanceID     = 0;
	gp.PassFlags          = 1u | (glow ? 2u : 0u);
	gp.InnerRadiusEncoded = static_cast<uint32>(std::clamp(mConquestInnerRadius, 0.f, 1.f) * 1000.f);
	gp.AlphaEncoded       = (alpha >= 0.999f) ? 0u : EncodeAlpha(alpha);

	gp.AnchorX  = posPx.x;
	gp.AnchorY  = posPx.y;
	gp.AnchorZ  = 0.f;
	gp.Progress = std::clamp(progress, 0.f, 1.f);

	gp.SizePxX  = sizePx.x;
	gp.SizePxY  = sizePx.y;
	gp.PivotPxX = -sizePx.x * 0.5f;
	gp.PivotPxY = -sizePx.y * 0.5f;

	gp.BackgroundTextureIndex = bgTex->GetImageIndex();
	gp.FillTextureIndex       = fillTex->GetImageIndex();

	GRAPHICS_CMD_LIST->SetGraphicsRoot32BitConstants(0, 16, &gp, 0);
	ringShader->Update();
	quadMesh->Render(1, 0, 0, 0);
}

// 리볼버 실린더식 배치
void UIPhaseProgressUpdateFeature::DrawConquestRoulette()
{
	auto ringShader = RESOURCEMANAGER.Get<Shader>(L"WorldUIConquestRing");
	auto quadMesh   = RESOURCEMANAGER.Get<Mesh>(L"UIQuad");

	if (ringShader == nullptr || quadMesh == nullptr)
		return;
	if (RESOURCEMANAGER.Get<Texture>(mConquestBgTextureName) == nullptr)
		return;

	int8 backIndex = RENDERMANAGER.GetSwapChain()->GetBackBufferIndex();
	RENDERMANAGER.GetRenderTargetGroup(static_cast<uint32>(RENDER_TARGET_GROUP_TYPE::SWAP_CHAIN))
		.OMSetRenderTargets(1, backIndex);

	RENDERMANAGER.SetGraphicsTable();

	const int32 zoneCount = std::max(1, mConquestZoneCount);
	const int32 current   = std::clamp(mCurrentZoneIdx, 0, zoneCount - 1);


	// EaseOutBack
	const float rollRemain = 1.f - MathUtils::EaseOutBack(mRollT);
	const float fadeSpan   = std::max(mConquestSlotFadeSpan, 1.001f);

	// 점령지가 1개
	if (zoneCount <= 1)
	{
		DrawConquestSlot(0.f, mConquestProgress, 1.f, false);
	}
	else
	{
		struct Slot { float offset; float progress; float alpha; };
		std::vector<Slot> slots;
		slots.reserve(zoneCount);

		for (int32 i = 0; i < zoneCount; ++i)
		{
			const float slotOffset = static_cast<float>(i - current) + rollRemain;


			const float dist = std::abs(slotOffset);

			if (dist >= fadeSpan)
				continue;

			const float alpha = 1.f - std::clamp((dist - 1.f) / (fadeSpan - 1.f), 0.f, 1.f);

			float progress;
			if      (i <  current) progress = 1.f;               // 점령 완료
			else if (i == current) progress = mConquestProgress; // 점령 진행 중
			else                   progress = 0.f;               // 점령 대기

			slots.push_back({ slotOffset, progress, alpha });
		}


		std::sort(slots.begin(), slots.end(),
			[](const Slot& a, const Slot& b) { return std::abs(a.offset) > std::abs(b.offset); });

		for (const Slot& slot : slots)
			DrawConquestSlot(slot.offset, slot.progress, slot.alpha, false);
	}

	// 완료 플래시
	if (mCompletedSlotIdx >= 0 && mCompletedSlotIdx < zoneCount && mRollT < 1.f)
	{
		const float slotOffset = static_cast<float>(mCompletedSlotIdx - current) + rollRemain;
		const float flashAlpha = 1.f - mRollT;       // 점점 투명
		const float flashScale = 1.f + 0.6f * mRollT; // 점점 확대
		DrawConquestSlot(slotOffset, 1.f, flashAlpha, true, flashScale);
	}


	const uint32 zeros[4] = { 0, 0, 0, 0 };
	GRAPHICS_CMD_LIST->SetGraphicsRoot32BitConstants(0, 4, zeros, 0);
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
		ImGui::DragFloat("SlotDiaOverRadius", &mConquestSlotDiaOverRadius, 0.005f, 0.05f, 2.f, "%.3f");
		ImGui::DragFloat("SideScale", &mConquestSideScale, 0.005f, 0.1f, 1.f, "%.3f");
		ImGui::Text("SlotDiaPx : center %.1f / side %.1f",
			GetConquestSlotBaseDiaPx(), GetConquestSlotBaseDiaPx() * mConquestSideScale);
		ImGui::SliderFloat("ConquestInnerRadius", &mConquestInnerRadius, 0.f, 1.f);

		mConquestSlotDiaOverRadius = std::clamp(mConquestSlotDiaOverRadius, 0.05f, 2.f);
		mConquestSideScale         = std::clamp(mConquestSideScale, 0.1f, 1.f);
		mConquestInnerRadius       = std::clamp(mConquestInnerRadius, 0.f, 1.f);

		ImGui::Separator();
		ImGui::Text("Background (최하단 레이어)");
		ImGui::Text("Texture : %s", RESOURCEMANAGER.Get<Texture>(mConquestBackTextureName) ? "loaded" : "(없음 - 배경 스킵)");
		ImGui::DragFloat2("BackAnchorRatio", &mConquestBackAnchorRatio.x, 0.001f, 0.f, 1.f, "%.4f");
		ImGui::DragFloat2("BackSizeRatio", &mConquestBackSizeRatio.x, 0.001f, 0.f, 1.f, "%.4f");

		mConquestBackAnchorRatio.x = std::clamp(mConquestBackAnchorRatio.x, 0.f, 1.f);
		mConquestBackAnchorRatio.y = std::clamp(mConquestBackAnchorRatio.y, 0.f, 1.f);
		mConquestBackSizeRatio.x   = std::clamp(mConquestBackSizeRatio.x, 0.f, 1.f);
		mConquestBackSizeRatio.y   = std::clamp(mConquestBackSizeRatio.y, 0.f, 1.f);

		ImGui::Separator();
		ImGui::Text("Roulette (하단 반원 배치)");
		ImGui::DragInt("ZoneCount", &mConquestZoneCount, 0.1f, 1, GameConquestComponent::mMaxConquest);
		// 배경 호의 X 눈금이 ±33도에 있다
		ImGui::DragFloat("ArcStepDegrees", &mConquestArcStepDegrees, 0.5f, 0.f, 90.f, "%.1f");

		// 호는 배경 텍스처의 링에서 유도되므로 직접 편집하지 않고 결과만 보여준다
		Vec2 arcCenterPx, arcRadiusPx;
		GetConquestArcGeometry(arcCenterPx, arcRadiusPx);
		ImGui::Text("ArcCenter(유도) : %.0f, %.0f   R : %.0f, %.0f",
			arcCenterPx.x, arcCenterPx.y, arcRadiusPx.x, arcRadiusPx.y);
		ImGui::Text("SlotPx  L:%.0f,%.0f  C:%.0f,%.0f  R:%.0f,%.0f",
			GetConquestSlotPosPx(-1.f).x, GetConquestSlotPosPx(-1.f).y,
			GetConquestSlotPosPx(0.f).x,  GetConquestSlotPosPx(0.f).y,
			GetConquestSlotPosPx(1.f).x,  GetConquestSlotPosPx(1.f).y);
		ImGui::DragFloat("SlotFadeSpan", &mConquestSlotFadeSpan, 0.01f, 1.01f, 4.f, "%.2f");
		ImGui::DragFloat("RollDuration", &mRollDuration, 0.01f, 0.f, 2.f, "%.2f");
		ImGui::Text("CurrentZone : %d / %d  (rollT %.2f)", mCurrentZoneIdx + 1, mConquestZoneCount, mRollT);
		// 켜지 않으면 서버 mActiveZoneId 가 매 프레임 슬롯을 덮어써서 Test Roll 이 즉시 되돌아간다
		ImGui::Checkbox("Debug Drive (서버 값 무시)", &mDebugDriveRoulette);
		if (ImGui::Button("Test Roll"))
		{
			// 다음 슬롯으로 한 칸 회전 (마지막이면 처음으로 되돌려 반복 확인)
			mCompletedSlotIdx = mCurrentZoneIdx;
			mCurrentZoneIdx   = (mCurrentZoneIdx + 1) % std::max(1, mConquestZoneCount);
			mRollT            = 0.f;
		}

		mConquestZoneCount      = std::clamp(mConquestZoneCount, 1, GameConquestComponent::mMaxConquest);
		mCurrentZoneIdx         = std::clamp(mCurrentZoneIdx, 0, mConquestZoneCount - 1);
		mConquestArcStepDegrees = std::clamp(mConquestArcStepDegrees, 0.f, 90.f);
		mConquestSlotFadeSpan   = std::clamp(mConquestSlotFadeSpan, 1.01f, 4.f);
		mRollDuration           = std::clamp(mRollDuration, 0.f, 2.f);
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

	// 일반 진행도 스프라이트 파라미터
	UIPhaseSpriteParamsLayout gp{};
	gp.BaseInstanceID  = 0;
	gp.PassFlags       = 1;
	gp.ReservedHeader = 0;
	const Vec2 anchorPx = GetProgressAnchorPx();
	
	gp.AnchorX  = anchorPx.x;
	gp.AnchorY  = anchorPx.y;
	gp.AnchorZ  = 0.f;
	gp.SizePxX  = escortSizePx.x;
	gp.SizePxY  = escortSizePx.y;
	gp.PivotPxX = -escortSizePx.x * 0.5f;
	gp.PivotPxY = -escortSizePx.y * 0.5f;
	gp.Progress = fillCutU;


	if (bgTex != nullptr)
	{
		gp.SpriteRole             = 0;
		gp.BackgroundTextureIndex = bgTex->GetImageIndex();
		gp.FillTextureIndex       = bgTex->GetImageIndex();
		GRAPHICS_CMD_LIST->SetGraphicsRoot32BitConstants(0, 16, &gp, 0);
		spriteShader->Update();
		quadMesh->Render(1, 0, 0, 0);
	}


	gp.SpriteRole             = 0;
	gp.BackgroundTextureIndex = lineTex->GetImageIndex();
	gp.FillTextureIndex       = checkTex->GetImageIndex();
	GRAPHICS_CMD_LIST->SetGraphicsRoot32BitConstants(0, 16, &gp, 0);
	spriteShader->Update();
	quadMesh->Render(1, 0, 0, 0);                     // line (role=0)

	const uint32 fillRole = 1;
	GRAPHICS_CMD_LIST->SetGraphicsRoot32BitConstants(0, 1, &fillRole, 2);
	quadMesh->Render(1, 0, 0, 0);                     // check (role=1, uv.x>fillCutU discard)

	// 커서
	if (cursorTex != nullptr)
	{
		gp.SpriteRole = 0;
		gp.SizePxX    = escortCursorSizePx.x;
		gp.SizePxY    = escortCursorSizePx.y;
		gp.PivotPxX   = -escortSizePx.x * 0.5f
		                   + fillCutU * escortSizePx.x
		                   - escortCursorSizePx.x * 0.5f;
		// 세로: 바와 같은 중앙선 정렬
		gp.PivotPxY               = -escortCursorSizePx.y * 0.5f;
		gp.BackgroundTextureIndex = cursorTex->GetImageIndex();
		gp.FillTextureIndex       = cursorTex->GetImageIndex();
		GRAPHICS_CMD_LIST->SetGraphicsRoot32BitConstants(0, 16, &gp, 0);
		spriteShader->Update();
		quadMesh->Render(1, 0, 0, 0);
	}


	const uint32 zero = 0;
	GRAPHICS_CMD_LIST->SetGraphicsRoot32BitConstants(0, 1, &zero, 0);
	GRAPHICS_CMD_LIST->SetGraphicsRoot32BitConstants(0, 1, &zero, 2);
}
