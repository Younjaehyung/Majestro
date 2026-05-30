#include "pch.h"
#include "Engine.h"
#include "ParticlePass.h"

#include "Buffer.h"
#include "Material.h"
#include "Mesh.h"
#include "ParticleSystem.h"
#include "RenderManager.h"
#include "ResourceManager.h"
#include "RenderTarget.h"
#include "Shader.h"
#include "World.h"


void ParticlePass::Initialize(World* world)
{
	mWorld = world;
	mParticleSystem = world ? world->GetSystemManager()->GetSystem<ParticleSystem>() : nullptr;
}

void ParticlePass::Execute(const RenderContext& ctx)
{
	if (mParticleSystem == nullptr)
		return;

	if (!mParticleSystem->HasActiveEmitters())
		return;

	const uint32 frameIndex = ctx.frameIndex;
	UploadSharedParams(frameIndex, ctx.deltaTime);
	auto spawnBuffer = RENDERMANAGER.GetParticleSpawnBuffer(frameIndex);
	if (spawnBuffer && spawnBuffer->GetResourceState() != D3D12_RESOURCE_STATE_UNORDERED_ACCESS)
	{
		auto barrier = CD3DX12_RESOURCE_BARRIER::Transition(
			spawnBuffer->GetBuffer().Get(),
			spawnBuffer->GetResourceState(),
			D3D12_RESOURCE_STATE_UNORDERED_ACCESS);
		GRAPHICS_CMD_LIST->ResourceBarrier(1, &barrier);
		spawnBuffer->SetResourceState(D3D12_RESOURCE_STATE_UNORDERED_ACCESS);
	}

	auto& hdrGroup = RENDERMANAGER.GetRenderTargetGroup(static_cast<uint32>(RENDER_TARGET_GROUP_TYPE::HDR));
	hdrGroup.WaitResourceToTarget();

	for (const ActiveParticleEmitter& emitter : mParticleSystem->GetActiveEmitters())
	{
		const ParticleEmitterRuntime* runtime = mParticleSystem->GetRuntime(emitter.entity);
		if (runtime == nullptr || runtime->IsValid() == false)
			continue;

		RunEmitterCompute(*runtime, emitter.emitterIndex, frameIndex);
		DrawEmitter(*runtime, emitter.emitterIndex);
	}

	hdrGroup.WaitTargetToResource();

	if (spawnBuffer && spawnBuffer->GetResourceState() != D3D12_RESOURCE_STATE_COMMON)
	{
		auto barrier = CD3DX12_RESOURCE_BARRIER::Transition(
			spawnBuffer->GetBuffer().Get(),
			spawnBuffer->GetResourceState(),
			D3D12_RESOURCE_STATE_COMMON);
		GRAPHICS_CMD_LIST->ResourceBarrier(1, &barrier);
		spawnBuffer->SetResourceState(D3D12_RESOURCE_STATE_COMMON);
	}
}

void ParticlePass::UploadSharedParams(uint32 frameIndex, float deltaTime)
{
	mSharedParams.clear();
	mSpawnParams.clear();

	const auto& emitters = mParticleSystem->GetActiveEmitters();
	mSharedParams.reserve(emitters.size());
	mSpawnParams.reserve(emitters.size());

	for (const ActiveParticleEmitter& emitter : emitters)
	{
		ParticleSharedParams params = emitter.sharedParams;
		params.deltaTime = deltaTime;
		mSharedParams.push_back(params);
		mSpawnParams.push_back({ params.addCount });
	}

	auto groupBuffer = RENDERMANAGER.GetGroupBuffer(frameIndex);
	groupBuffer->ParticleInfo->UpdateDefaultFromCpu(
		mSharedParams.data(),
		static_cast<uint32>(mSharedParams.size() * sizeof(ParticleSharedParams)));

	auto spawnBuffer = RENDERMANAGER.GetParticleSpawnBuffer(frameIndex);
	spawnBuffer->UpdateDefaultFromCpu(
		mSpawnParams.data(),
		static_cast<uint32>(mSpawnParams.size() * sizeof(ParticleComputeSharedParams)));
}

void ParticlePass::RunEmitterCompute(const ParticleEmitterRuntime& runtime, uint32 emitterIndex, uint32 frameIndex)
{
	auto particleBuffer = runtime.gpu.particleBuffer;
	if (particleBuffer == nullptr)
		return;

	if (particleBuffer->GetResourceState() != D3D12_RESOURCE_STATE_UNORDERED_ACCESS)
	{
		auto barrier = CD3DX12_RESOURCE_BARRIER::Transition(
			particleBuffer->GetBuffer().Get(),
			particleBuffer->GetResourceState(),
			D3D12_RESOURCE_STATE_UNORDERED_ACCESS);
		GRAPHICS_CMD_LIST->ResourceBarrier(1, &barrier);
		particleBuffer->SetResourceState(D3D12_RESOURCE_STATE_UNORDERED_ACCESS);
	}

	ID3D12DescriptorHeap* descHeap = RENDERMANAGER.GetGraphicsDescHeap()->GetDescriptorHeap().Get();
	GRAPHICS_CMD_LIST->SetDescriptorHeaps(1, &descHeap);

	auto rootSignature = RESOURCEMANAGER.Get<RootSignature>(L"MainRootSignature");
	GRAPHICS_CMD_LIST->SetComputeRootSignature(rootSignature->GetRootSignature().Get());

	ParticlePassRootConstants constants{};
	constants.EmitterIndex = emitterIndex;
	GRAPHICS_CMD_LIST->SetComputeRoot32BitConstants(0, 4, &constants, 0);
	GRAPHICS_CMD_LIST->SetComputeRootDescriptorTable(mParticleRootGroup, GetGpuHandle(GROUP_START + (frameIndex * GROUP_COUNT)));
	GRAPHICS_CMD_LIST->SetComputeRootDescriptorTable(mParticleRootRuntime, GetGpuHandle(runtime.descriptorBlock.startIndex));
	GRAPHICS_CMD_LIST->SetComputeRootDescriptorTable(mParticleRootSpawn, GetGpuHandle(PARTICLE_SPAWN_INDEX_START + frameIndex));

	runtime.computeShader->Update();

	const uint32 dispatchCount = (runtime.gpu.capacity + 1023) / 1024;
	GRAPHICS_CMD_LIST->Dispatch(dispatchCount, 1, 1);

	auto uavBarrier = CD3DX12_RESOURCE_BARRIER::UAV(particleBuffer->GetBuffer().Get());
	GRAPHICS_CMD_LIST->ResourceBarrier(1, &uavBarrier);

	auto toCommon = CD3DX12_RESOURCE_BARRIER::Transition(
		particleBuffer->GetBuffer().Get(),
		D3D12_RESOURCE_STATE_UNORDERED_ACCESS,
		D3D12_RESOURCE_STATE_COMMON);
	GRAPHICS_CMD_LIST->ResourceBarrier(1, &toCommon);
	particleBuffer->SetResourceState(D3D12_RESOURCE_STATE_COMMON);
}

void ParticlePass::DrawEmitter(const ParticleEmitterRuntime& runtime, uint32 emitterIndex)
{
	auto& hdrGroup = RENDERMANAGER.GetRenderTargetGroup(static_cast<uint32>(RENDER_TARGET_GROUP_TYPE::HDR));
	hdrGroup.OMSetRenderTargetsReadOnlyDepth();

	RENDERMANAGER.SetGraphicsTable();
	GRAPHICS_CMD_LIST->SetGraphicsRootDescriptorTable(mParticleRootRuntime, GetGpuHandle(runtime.descriptorBlock.startIndex));

	ParticlePassRootConstants constants{};
	constants.EmitterIndex = emitterIndex;
	GRAPHICS_CMD_LIST->SetGraphicsRoot32BitConstants(0, 4, &constants, 0);

	runtime.material->GetShader()->Update();
	runtime.mesh->Render(runtime.gpu.capacity);
}

D3D12_GPU_DESCRIPTOR_HANDLE ParticlePass::GetGpuHandle(uint32 descriptorIndex) const
{
	D3D12_GPU_DESCRIPTOR_HANDLE handle = RENDERMANAGER.GetGraphicsDescHeap()->GetDescriptorHeap()->GetGPUDescriptorHandleForHeapStart();
	const uint32 incrementSize = DEVICE->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
	handle.ptr += static_cast<uint64>(descriptorIndex) * incrementSize;
	return handle;
}
