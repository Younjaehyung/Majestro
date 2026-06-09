#include "pch.h"
#include "LobbyBackgroundPass.h"

#include "Engine.h"
#include "RenderManager.h"
#include "ResourceManager.h"

void LobbyBackgroundPass::Initialize()
{
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

	RESOURCEMANAGER.Get<Shader>(L"LobbyBackground")->Update();
	RESOURCEMANAGER.Get<Mesh>(L"Rectangle")->Render();

	hdrGroup.WaitTargetToResource();

	auto toDepthWrite = CD3DX12_RESOURCE_BARRIER::Transition(
		depthResource,
		D3D12_RESOURCE_STATE_DEPTH_READ | D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE,
		D3D12_RESOURCE_STATE_DEPTH_WRITE);
	GRAPHICS_CMD_LIST->ResourceBarrier(1, &toDepthWrite);
}
