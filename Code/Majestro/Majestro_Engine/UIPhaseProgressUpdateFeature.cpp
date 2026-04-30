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
	if (!gameRuleComp)
	{
		mCachedConquestProgress = -1.f;
		return;
	}

	switch (gameRuleComp->mGamePhase)
	{
		case uint8(WavePhaseType::Prepare): // Prepare
			mCachedConquestProgress = -1.f;
			break;
		case uint8(WavePhaseType::Conquest): { // Conquest

			GameConquestComponent* gameConquestComp = mWorld->GetComponent<GameConquestComponent>(e);
			if (gameConquestComp) UpdateConquestProgress(dt, gameConquestComp);
			else                  mCachedConquestProgress = -1.f;
			break;
		}
		case uint8(WavePhaseType::Escort): { // Escort
			mCachedConquestProgress = -1.f;
			GameEscortComponent* gameEscortComp = mWorld->GetComponent<GameEscortComponent>(e);
			if (gameEscortComp) UpdateEscortProgress(dt, gameEscortComp);
			break;
		}
		default:
			mCachedConquestProgress = -1.f;
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
	if (mCachedConquestProgress < 0.f)
		return;

	DrawConquestRing();
}

// Phase Update

void UIPhaseProgressUpdateFeature::UpdateConquestProgress(float dt, GameConquestComponent* conquestComp)
{
	const float total = GameConquestComponent::mMaxWaveTime;
	const float ratio = (total > 0.f) ? (conquestComp->mWaveTime / total) : 0.f;
	mCachedConquestProgress       = std::clamp(ratio, 0.f, 1.f);
	mCachedConquestWaveCheckPoint = conquestComp->mWaveCheckPoint;
}

void UIPhaseProgressUpdateFeature::UpdateEscortProgress(float dt, GameEscortComponent* escortComp)
{
}

// Draw

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
	gp.HpBarAnchorWorldX = mConquestScreenAnchorPx.x;
	gp.HpBarAnchorWorldY = mConquestScreenAnchorPx.y;
	gp.HpBarAnchorWorldZ = 0.f;
	gp.HpBarFollowRatio  = std::clamp(mCachedConquestProgress, 0.f, 1.f);

	gp.HpBarSizePxX = mConquestSizePx.x;
	gp.HpBarSizePxY = mConquestSizePx.y;
	gp.HpBarPivotPxX = -mConquestSizePx.x * 0.5f;
	gp.HpBarPivotPxY = -mConquestSizePx.y * 0.5f;

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
