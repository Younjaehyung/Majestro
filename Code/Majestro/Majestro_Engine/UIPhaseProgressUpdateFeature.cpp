#include "pch.h"
#include "UIPhaseProgressUpdateFeature.h"
#include "GameRuleComponent.h"
#include "GameMode.h"

#include "Engine.h"
#include "RenderManager.h"
#include "ResourceManager.h"
#include "Mesh.h"
#include "Shader.h"
#include "Texture.h"
#include "World.h"
#include "UIHpBarUpdateFeature.h" 

void UIPhaseProgressUpdateFeature::Initialize(World* world)
{
	UIFeature::Initialize(world);
}

void UIPhaseProgressUpdateFeature::Update(float dt)
{
	Entity e = mWorld->GetGameRuleEntity();

	GameRuleComponent* gameRuleComp = mWorld->GetComponent<GameRuleComponent>(e);

	std::cout << "UIPhaseProgressUpdateFeature::Update - Game Phase: " << static_cast<int>(gameRuleComp->mGamePhase) << std::endl;
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


	Entity e = mWorld->GetGameRuleEntity();

	GameRuleComponent* gameRuleComp = mWorld->GetComponent<GameRuleComponent>(e);

	switch (gameRuleComp->mGamePhase)
	{
	case uint8(WavePhaseType::Prepare): // Prepare

		break;
	case uint8(WavePhaseType::Conquest): { // Conquest

		GameConquestComponent* gameConquestComp = mWorld->GetComponent<GameConquestComponent>(e);
		if (gameConquestComp) DrawConquestRing();
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
}

void UIPhaseProgressUpdateFeature::UpdateEscortProgress(float dt, GameEscortComponent* escortComp)
{

	float progress = std::clamp(escortComp->mEscortProgress, 0.f, 1.f);

	mEscortProgress = progress;// static_cast<float>(escortComp->mEscortStage) + (progress);

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
	gp.etc            = 1; // HUD 모드
	// casdcae 에 innerRadius * 1000 을 정수로 인코딩 (PS 에서 / 1000.0 으로 복원)
	gp.casdcae        = static_cast<uint32>(std::clamp(mConquestInnerRadius, 0.f, 1.f) * 1000.f);
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
	}

	if (ImGui::CollapsingHeader("Escort", ImGuiTreeNodeFlags_DefaultOpen))
	{
		ImGui::DragFloat2("EscortSizeRatio", &mEscortSizeRatio.x, 0.001f, 0.f, 1.f, "%.4f");
		ImGui::Text("EscortSizePx : %.1f, %.1f", escortSizePx.x, escortSizePx.y);
		ImGui::DragFloat2("EscortCursorSizeRatio", &mEscortCursorSizeRatio.x, 0.001f, 0.f, 1.f, "%.4f");
		ImGui::Text("EscortCursorSizePx : %.1f, %.1f", escortCursorSizePx.x, escortCursorSizePx.y);


		mEscortSizeRatio.x = std::clamp(mEscortSizeRatio.x, 0.f, 1.f);
		mEscortSizeRatio.y = std::clamp(mEscortSizeRatio.y, 0.f, 1.f);
		mEscortCursorSizeRatio.x = std::clamp(mEscortCursorSizeRatio.x, 0.f, 1.f);
		mEscortCursorSizeRatio.y = std::clamp(mEscortCursorSizeRatio.y, 0.f, 1.f);
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

	// mEscortProgress = stage + 0~1 → 현재 스테이지의 채움 비율만 추출
	const float fillRatio = std::clamp(mEscortProgress - std::floor(mEscortProgress), 0.f, 1.f);
	const Vec2 escortSizePx = GetProgressSizePx(mEscortSizeRatio);
	const Vec2 escortCursorSizePx = GetProgressSizePx(mEscortCursorSizeRatio);

	// 공통 GlobalParams (HUD 모드, 앵커는 바 정중앙)
	GlobalParamsLayout gp{};
	gp.BaseInstanceID    = 0;
	gp.etc               = 1; // HUD 모드 (스크린 스페이스, depth occlusion 스킵)
	gp.PassCustomIndex   = 0;
	const Vec2 anchorPx = GetProgressAnchorPx();
	
	gp.HpBarAnchorWorldX = anchorPx.x;
	gp.HpBarAnchorWorldY = anchorPx.y;
	gp.HpBarAnchorWorldZ = 0.f;
	gp.HpBarSizePxX      = escortSizePx.x;
	gp.HpBarSizePxY      = escortSizePx.y;
	gp.HpBarPivotPxX     = -escortSizePx.x * 0.5f;
	gp.HpBarPivotPxY     = -escortSizePx.y * 0.5f;
	gp.HpBarFollowRatio  = fillRatio;
	gp.HpBarHitTexIdx    = 0;
	gp.HpBarHitConfig    = 0;

	// 1) BG (가장 뒤, 풀 출력) — role=0 → BgTexIdx 사용
	if (bgTex != nullptr)
	{
		gp.casdcae         = 0;
		gp.HpBarBgTexIdx   = bgTex->GetImageIndex();
		gp.HpBarFillTexIdx = bgTex->GetImageIndex();
		GRAPHICS_CMD_LIST->SetGraphicsRoot32BitConstants(0, 16, &gp, 0);
		spriteShader->Update();
		quadMesh->Render(1, 0, 0, 0);
	}

	// 2) Line (빈 트랙) + 3) Check (채움) — 한 번의 GlobalParams 셋업으로 두 패스
	gp.casdcae         = 0;
	gp.HpBarBgTexIdx   = lineTex->GetImageIndex();
	gp.HpBarFillTexIdx = checkTex->GetImageIndex();
	GRAPHICS_CMD_LIST->SetGraphicsRoot32BitConstants(0, 16, &gp, 0);
	spriteShader->Update();
	quadMesh->Render(1, 0, 0, 0);                     // line (role=0)

	const uint32 fillRole = 1;
	GRAPHICS_CMD_LIST->SetGraphicsRoot32BitConstants(0, 1, &fillRole, 2);
	quadMesh->Render(1, 0, 0, 0);                     // check (role=1, uv.x>fillRatio discard)

	// 4) Cursor — 같은 앵커, Pivot 만 진행도 위치로 평행 이동
	if (cursorTex != nullptr)
	{
		gp.casdcae         = 0;
		gp.HpBarSizePxX    = escortCursorSizePx.x;
		gp.HpBarSizePxY    = escortCursorSizePx.y;
		// 가로: 바 좌끝(-w/2) + fillRatio * w 위치에 커서 가운데 정렬
		gp.HpBarPivotPxX   = -escortSizePx.x * 0.5f
		                   + fillRatio * escortSizePx.x
		                   - escortCursorSizePx.x * 0.5f;
		// 세로: 바와 같은 중앙선 정렬
		gp.HpBarPivotPxY   = -escortCursorSizePx.y * 0.5f;
		gp.HpBarBgTexIdx   = cursorTex->GetImageIndex();
		gp.HpBarFillTexIdx = cursorTex->GetImageIndex();
		GRAPHICS_CMD_LIST->SetGraphicsRoot32BitConstants(0, 16, &gp, 0);
		spriteShader->Update();
		quadMesh->Render(1, 0, 0, 0);
	}

	// 후속 패스 보호 — DrawConquestRing 과 동일하게 BaseInstanceID / casdcae 원복
	const uint32 zero = 0;
	GRAPHICS_CMD_LIST->SetGraphicsRoot32BitConstants(0, 1, &zero, 0);
	GRAPHICS_CMD_LIST->SetGraphicsRoot32BitConstants(0, 1, &zero, 2);
}
