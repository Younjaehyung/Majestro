#include "pch.h"
#include "WeaponTrailPass.h"

#include "Engine.h"
#include "RenderManager.h"
#include "ResourceManager.h"
#include "RenderTarget.h"
#include "Shader.h"
#include "Texture.h"
#include "WeaponTrailComponent.h"
#include "World.h"

void WeaponTrailPass::Initialize(World* world)
{
	mWorld = world;
	
	mShader = RESOURCEMANAGER.Get<Shader>(L"WeaponTrail");
}

void WeaponTrailPass::Execute(const RenderContext& ctx)
{
	if (mWorld == nullptr || mShader == nullptr)
		return;

	if (mWorld->HasComponentPool<WeaponTrailComponent>() == false)
		return;

	std::vector<Vertex> vertices;
	std::vector<uint32> indices;

	auto view = mWorld->View<WeaponTrailComponent>();
	for (Entity entity : view)
	{
		const WeaponTrailComponent* trail = mWorld->GetComponent<WeaponTrailComponent>(entity);
		if (trail == nullptr || trail->mIsActive == false)
			continue;

		BuildTrailMesh(*trail, vertices, indices);
	}

	if (vertices.empty() || indices.empty())
		return;

	auto vertexMemory = RENDERMANAGER.GetGraphicsMemory()->Allocate(
		vertices.size() * sizeof(Vertex),
		alignof(Vertex),
		DirectX::DX12::GraphicsMemory::TAG_VERTEX);
	::memcpy(vertexMemory.Memory(), vertices.data(), vertices.size() * sizeof(Vertex));

	auto indexMemory = RENDERMANAGER.GetGraphicsMemory()->Allocate(
		indices.size() * sizeof(uint32),
		alignof(uint32),
		DirectX::DX12::GraphicsMemory::TAG_INDEX);
	::memcpy(indexMemory.Memory(), indices.data(), indices.size() * sizeof(uint32));

	D3D12_VERTEX_BUFFER_VIEW vertexBufferView{};
	vertexBufferView.BufferLocation = vertexMemory.GpuAddress();
	vertexBufferView.SizeInBytes = static_cast<uint32>(vertices.size() * sizeof(Vertex));
	vertexBufferView.StrideInBytes = sizeof(Vertex);

	D3D12_INDEX_BUFFER_VIEW indexBufferView{};
	indexBufferView.BufferLocation = indexMemory.GpuAddress();
	indexBufferView.SizeInBytes = static_cast<uint32>(indices.size() * sizeof(uint32));
	indexBufferView.Format = DXGI_FORMAT_R32_UINT;

	auto& hdrGroup = RENDERMANAGER.GetRenderTargetGroup(static_cast<uint32>(RENDER_TARGET_GROUP_TYPE::HDR));
	hdrGroup.WaitResourceToTarget();
	hdrGroup.OMSetRenderTargetsReadOnlyDepth();


	RENDERMANAGER.SetGraphicsTable();
	mShader->Update();
	GRAPHICS_CMD_LIST->IASetVertexBuffers(0, 1, &vertexBufferView);
	GRAPHICS_CMD_LIST->IASetIndexBuffer(&indexBufferView);
	GRAPHICS_CMD_LIST->DrawIndexedInstanced(static_cast<uint32>(indices.size()), 1, 0, 0, 0);

	hdrGroup.WaitTargetToResource();
}

void WeaponTrailPass::BuildTrailMesh(const WeaponTrailComponent& trail, std::vector<Vertex>& vertices, std::vector<uint32>& indices) const
{
	const size_t sampleCount = trail.mSamples.size();
	if (sampleCount < 2)
		return;

	const uint32 baseVertex = static_cast<uint32>(vertices.size());
	const float lifetime = max(trail.mLifetime, 0.001f);
	const float lastIndex = static_cast<float>(sampleCount - 1);

	// WeaponTrail 텍스처:
	// 트레일 텍스처 키 : weights.x 채널
	float textureIndex = -1.0f;
	if (trail.mTextureName.empty() == false)
	{
		if (shared_ptr<Texture> texture = RESOURCEMANAGER.Get<Texture>(trail.mTextureName))
			textureIndex = static_cast<float>(texture->GetImageIndex());
	}

	vertices.reserve(vertices.size() + sampleCount * 2);
	indices.reserve(indices.size() + (sampleCount - 1) * 6);

	for (size_t i = 0; i < sampleCount; ++i)
	{
		const WeaponTrailSample& sample = trail.mSamples[i];
		const float pathRate = static_cast<float>(i) / lastIndex;
		const float ageRate = std::clamp(sample.Age / lifetime, 0.0f, 1.0f);
		const float alpha = trail.mBaseAlpha * (1.0f - ageRate);

		Vertex baseVertexData{};
		baseVertexData.pos = sample.BaseWorld;
		baseVertexData.uv = Vec2(0.0f, pathRate);
		baseVertexData.normal = trail.mEdgeColor;
		baseVertexData.tangent = Vec3(alpha, ageRate, trail.mIntensity);
		baseVertexData.weights.x = textureIndex;

		Vertex tipVertexData{};
		tipVertexData.pos = sample.TipWorld;
		tipVertexData.uv = Vec2(1.0f, pathRate);
		tipVertexData.normal = trail.mCoreColor;
		tipVertexData.tangent = Vec3(alpha, ageRate, trail.mIntensity);
		tipVertexData.weights.x = textureIndex;

		vertices.push_back(baseVertexData);
		vertices.push_back(tipVertexData);
	}

	for (size_t i = 0; i + 1 < sampleCount; ++i)
	{
		const uint32 i0 = baseVertex + static_cast<uint32>(i * 2);
		const uint32 i1 = i0 + 1;
		const uint32 i2 = i0 + 2;
		const uint32 i3 = i0 + 3;

		indices.push_back(i0);
		indices.push_back(i2);
		indices.push_back(i1);
		indices.push_back(i1);
		indices.push_back(i2);
		indices.push_back(i3);
	}
}
