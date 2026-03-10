#include "pch.h"
#include "FinalCompositePass.h"

#include "RenderManager.h"
#include "Engine.h"
#include "ResourceManager.h"

#include "ToneMapPass.h"
#include "ChromaticAberrationPass.h"

void FinalCompositePass::Execute(RENDER_TARGET_GROUP_TYPE before)
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
		RENDERMANAGER.GetRenderTargetGroup(static_cast<uint32>(RENDER_TARGET_GROUP_TYPE::SWAP_CHAIN)).OMSetRenderTargets(1,backIndex);
	}

	// HDR RT는 EffectPass에서 WaitTargetToResource()로 SRV 상태
	RESOURCEMANAGER.Get<Shader>(L"FianlComposite")->Update();
	RESOURCEMANAGER.Get<Mesh>(L"Rectangle")->Render();

	if (RENDERMANAGER.IsMsaaEnabled())
		RENDERMANAGER.GetRenderTargetGroup(static_cast<uint32>(RENDER_TARGET_GROUP_TYPE::MSAA_SWAP_CHAIN)).WaitTargetToResource();

}
