#include "pch.h"
#include "LobbyBackgroundPass.h"

#include "Engine.h"
#include "PlayerComponent.h"
#include "RenderManager.h"
#include "ResourceManager.h"
#include "TagComponent.h"
#include "World.h"

namespace
{
	Vec4 GetLobbyBackgroundTheme(uint8 playerType)
	{
		// 각 캐릭터의 대표색과 배경에 적용할 혼합 강도를 함께 반환한다
		switch (static_cast<PlayerType>(playerType))
		{
		case PlayerType::Rudwig:
			return Vec4(1.00f, 0.45f, 0.12f, 0.68f);
		case PlayerType::Ibanix:
			return Vec4(0.15f, 1.00f, 0.35f, 0.68f);
		case PlayerType::Fanthor:
			return Vec4(0.72f, 0.25f, 1.00f, 0.68f);
		default:
			return Vec4(1.00f, 1.00f, 1.00f, 0.00f);
		}
	}
}

void LobbyBackgroundPass::Initialize(World* world)
{
	mWorld = world;
}

void LobbyBackgroundPass::SetData(
	std::array<PassCustomData, static_cast<uint32>(PASS_CUSTOM_INDEX::PASS_CUSTOM_COUNT)>& dataTable,
	RENDER_TARGET_GROUP_TYPE before,
	RENDER_TARGET_GROUP_TYPE after)
{
	mBefore = before;
	mAfter = after;

	uint8 selectedPlayerType = static_cast<uint8>(PlayerType::Count);

	// 로컬 플레이어가 현재 선택한 캐릭터 타입을 로비 월드에서 읽는다
	if (mWorld != nullptr && mWorld->HasComponentPool<ChoicePlayerComponent>())
	{
		const auto choiceEntities = mWorld->GetEntitiesWithComponent<ChoicePlayerComponent>();
		if (!choiceEntities.empty())
		{
			if (const auto* choice = mWorld->GetComponent<ChoicePlayerComponent>(choiceEntities[0]))
				selectedPlayerType = choice->mPlayerType;
		}
	}

	// rgb에는 테마 색상을 넣고 a에는 기존 배경과 혼합할 강도를 넣는다
	auto& passData = dataTable[static_cast<uint32>(PASS_CUSTOM_INDEX::LOBBY_BACKGROUND_PASS)];
	passData.ExtValue[0] = GetLobbyBackgroundTheme(selectedPlayerType);
}

void LobbyBackgroundPass::Execute(std::vector<DrawBatch>& deferredDrawBatchs)
{
	if (!mEnabled)
		return;

	auto& hdrGroup = RENDERMANAGER.GetRenderTargetGroup(static_cast<uint32>(RENDER_TARGET_GROUP_TYPE::HDR));
	auto depthTex = hdrGroup.GetDSTexture();
	auto* depthResource = depthTex->GetTex2D().Get();

	auto toDepthRead = CD3DX12_RESOURCE_BARRIER::Transition(
		depthResource,
		D3D12_RESOURCE_STATE_DEPTH_WRITE,
		D3D12_RESOURCE_STATE_DEPTH_READ | D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);
	GRAPHICS_CMD_LIST->ResourceBarrier(1, &toDepthRead);

	hdrGroup.WaitResourceToTarget();
	hdrGroup.OMSetRenderTargetsReadOnlyDepth();

	// 이 드로우에서 사용할 로비 배경 전용 데이터를 선택한다
	const uint32 passCustomIndex = static_cast<uint32>(PASS_CUSTOM_INDEX::LOBBY_BACKGROUND_PASS);
	GRAPHICS_CMD_LIST->SetGraphicsRoot32BitConstants(0, 1, &passCustomIndex, 3);

	RESOURCEMANAGER.Get<Shader>(L"LobbyBackground")->Update();
	RESOURCEMANAGER.Get<Mesh>(L"Rectangle")->Render();

	hdrGroup.WaitTargetToResource();

	auto toDepthWrite = CD3DX12_RESOURCE_BARRIER::Transition(
		depthResource,
		D3D12_RESOURCE_STATE_DEPTH_READ | D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE,
		D3D12_RESOURCE_STATE_DEPTH_WRITE);
	GRAPHICS_CMD_LIST->ResourceBarrier(1, &toDepthWrite);
}
