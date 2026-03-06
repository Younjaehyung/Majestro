#include "pch.h"
#include "ToneMapPass.h"

#include "Engine.h"
#include "RenderManager.h"
#include "ResourceManager.h"

void ToneMapPass::Execute()
{
	int8 backIndex = RENDERMANAGER.GetSwapChain()->GetBackBufferIndex();

	if (RENDERMANAGER.IsMsaaEnabled())
	{
		auto& finalGroup = RENDERMANAGER.GetRenderTargetGroup(static_cast<uint32>(RENDER_TARGET_GROUP_TYPE::MSAA_SWAP_CHAIN));
		finalGroup.WaitResourceToTarget();
		finalGroup.OMSetRenderTargets(1, backIndex);
	}
	else
	{
		RENDERMANAGER.GetRenderTargetGroup(static_cast<uint32>(RENDER_TARGET_GROUP_TYPE::SWAP_CHAIN)).OMSetRenderTargets(1, backIndex);
	}

	// HDR RT는 EffectPass에서 WaitTargetToResource()로 SRV 상태
	RESOURCEMANAGER.Get<Shader>(L"ToneMap")->Update();
	RESOURCEMANAGER.Get<Mesh>(L"Rectangle")->Render();

	if (RENDERMANAGER.IsMsaaEnabled())
		RENDERMANAGER.GetRenderTargetGroup(static_cast<uint32>(RENDER_TARGET_GROUP_TYPE::MSAA_SWAP_CHAIN)).WaitTargetToResource();

	// SwapChain은 RT 상태 유지 — UIRenderSystem이 이어서 렌더링
}
