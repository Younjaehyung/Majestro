#include "pch.h"
#include "TrailPass.h"

#include "Engine.h"
#include "RenderManager.h"
#include "RenderTarget.h"
#include "ResourceManager.h"
#include "Shader.h"
#include "Texture.h"
#include "WeaponTrailComponent.h"

namespace
{
	int32 ResolveTrailTextureIndex(WeaponTrailComponent& trail)
	{
		if (trail.mTextureIndex >= 0)
			return trail.mTextureIndex;
		if (trail.mTextureName.empty())
			return -1;

		shared_ptr<Texture> texture = RESOURCEMANAGER.Get<Texture>(trail.mTextureName);
		if (texture == nullptr)
			return -1;

		// 수정: 컴포넌트는 Texture 리소스 키만 들고 있고, 렌더 패스에서 실제 TextureMaps[] 인덱스로 1회 변환해 캐시한다.
		trail.mTextureIndex = static_cast<int32>(texture->GetImageIndex());
		return trail.mTextureIndex;
	}
}

void TrailPass::Initialize(World* world)
{
	mWorld = world;
	mShader = RESOURCEMANAGER.Get<Shader>(L"Trail");
	CreateBuffers();
}


void TrailPass::CreateBuffers()
{
	const uint32 vertexBufferSize = MAX_TRAIL_VERTICES * static_cast<uint32>(sizeof(Vertex));
	const uint32 indexBufferSize = MAX_TRAIL_INDICES * static_cast<uint32>(sizeof(uint16));

	D3D12_HEAP_PROPERTIES heapProp = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD);

	{
		D3D12_RESOURCE_DESC desc = CD3DX12_RESOURCE_DESC::Buffer(vertexBufferSize);
		DEVICE->CreateCommittedResource(
			&heapProp,
			D3D12_HEAP_FLAG_NONE,
			&desc,
			D3D12_RESOURCE_STATE_GENERIC_READ,
			nullptr,
			IID_PPV_ARGS(&mVertexBuffer));

		CD3DX12_RANGE readRange(0, 0);
		mVertexBuffer->Map(0, &readRange, reinterpret_cast<void**>(&mMappedVertices));

		mVBV.BufferLocation = mVertexBuffer->GetGPUVirtualAddress();
		mVBV.StrideInBytes = sizeof(Vertex);
		mVBV.SizeInBytes = vertexBufferSize;
	}

	{
		D3D12_RESOURCE_DESC desc = CD3DX12_RESOURCE_DESC::Buffer(indexBufferSize);
		DEVICE->CreateCommittedResource(
			&heapProp,
			D3D12_HEAP_FLAG_NONE,
			&desc,
			D3D12_RESOURCE_STATE_GENERIC_READ,
			nullptr,
			IID_PPV_ARGS(&mIndexBuffer));

		CD3DX12_RANGE readRange(0, 0);
		mIndexBuffer->Map(0, &readRange, reinterpret_cast<void**>(&mMappedIndices));

		mIBV.BufferLocation = mIndexBuffer->GetGPUVirtualAddress();
		mIBV.Format = DXGI_FORMAT_R16_UINT;
		mIBV.SizeInBytes = indexBufferSize;
	}
}

void TrailPass::FillTrailVertex(Vertex& outVertex, const Vec3& worldPos, const Vec2& uv, const Vec4& color,
	int32 textureIndex, float textureAlphaWeight, bool useTextureColor)
{
	outVertex = {};
	outVertex.pos = worldPos;
	outVertex.uv = uv;

	// 수정: 엔진의 고정 Vertex 레이아웃을 그대로 쓰기 위해 trail 색상은 normal.xyz,
	// alpha는 tangent.x에 패킹한다. 전용 TrailVertex를 만들면 더 깔끔하지만 영향 범위가 커진다.
	outVertex.normal = Vec3(color.x, color.y, color.z);
	// 수정: 엔진 공용 Vertex 레이아웃을 유지하면서 trail 전용 데이터를 남는 tangent 채널에 패킹한다.
	// tangent.x = alpha, tangent.y = TextureMaps[] 인덱스, tangent.z = 텍스처 적용 옵션이다.
	outVertex.tangent = Vec3(color.w, static_cast<float>(textureIndex),
		useTextureColor ? -textureAlphaWeight : textureAlphaWeight);
}

void TrailPass::Execute()
{
	if (mWorld == nullptr || mShader == nullptr || mMappedVertices == nullptr || mMappedIndices == nullptr)
		return;
	if (mWorld->HasComponentPool<WeaponTrailComponent>() == false)
		return;

	uint32 vertexCount = 0;
	uint32 indexCount = 0;

	for (Entity entity : mWorld->View<WeaponTrailComponent>())
	{
		WeaponTrailComponent* trail = mWorld->GetComponent<WeaponTrailComponent>(entity);
		if (trail == nullptr)
			continue;
		if (trail->mSamples.size() < 2)
			continue;

		const uint32 sampleCount = static_cast<uint32>(trail->mSamples.size());
		const uint32 requiredVertices = sampleCount * 2;
		const uint32 requiredIndices = (sampleCount - 1) * 6;
		if (vertexCount + requiredVertices > MAX_TRAIL_VERTICES)
			break;
		if (indexCount + requiredIndices > MAX_TRAIL_INDICES)
			break;

		const uint16 trailBaseVertex = static_cast<uint16>(vertexCount);
		const float lifeTime = max(trail->mLifeTime, 0.0001f);
		const float sampleDenom = max(1u, sampleCount - 1);
		const int32 textureIndex = ResolveTrailTextureIndex(*trail);
		const float textureAlphaWeight = std::clamp(trail->mTextureAlphaWeight, 0.f, 1.f);

		for (uint32 i = 0; i < sampleCount; ++i)
		{
			const TrailSample& sample = trail->mSamples[i];
			const float ageAlpha = std::clamp(1.f - (sample.age / lifeTime), 0.f, 1.f);
			const float along = static_cast<float>(i) / static_cast<float>(sampleDenom);
			Vec4 color = trail->mColor;
			color.w *= ageAlpha;

			FillTrailVertex(mMappedVertices[vertexCount + 0], sample.basePos, Vec2(0.f, along), color,
				textureIndex, textureAlphaWeight, trail->mUseTextureColor);
			FillTrailVertex(mMappedVertices[vertexCount + 1], sample.tipPos, Vec2(1.f, along), color,
				textureIndex, textureAlphaWeight, trail->mUseTextureColor);
			vertexCount += 2;
		}

		for (uint32 i = 0; i < sampleCount - 1; ++i)
		{
			const uint16 i0 = trailBaseVertex + static_cast<uint16>(i * 2 + 0);
			const uint16 i1 = trailBaseVertex + static_cast<uint16>(i * 2 + 1);
			const uint16 i2 = trailBaseVertex + static_cast<uint16>(i * 2 + 2);
			const uint16 i3 = trailBaseVertex + static_cast<uint16>(i * 2 + 3);

			mMappedIndices[indexCount + 0] = i0;
			mMappedIndices[indexCount + 1] = i1;
			mMappedIndices[indexCount + 2] = i2;
			mMappedIndices[indexCount + 3] = i2;
			mMappedIndices[indexCount + 4] = i1;
			mMappedIndices[indexCount + 5] = i3;
			indexCount += 6;
		}
	}

	if (indexCount == 0)
		return;

	auto& hdrGroup = RENDERMANAGER.GetRenderTargetGroup(static_cast<uint32>(RENDER_TARGET_GROUP_TYPE::HDR));
	auto depthTex = hdrGroup.GetDSTexture();
	ID3D12Resource* depthResource = depthTex ? depthTex->GetTex2D().Get() : nullptr;
	if (depthResource != nullptr)
	{
		// 수정: TrailPass는 depth test만 필요하고 depth write는 하지 않으므로 read-only DSV에 맞춰 상태를 전환한다.
		auto toDepthRead = CD3DX12_RESOURCE_BARRIER::Transition(
			depthResource,
			D3D12_RESOURCE_STATE_DEPTH_WRITE,
			D3D12_RESOURCE_STATE_DEPTH_READ | D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);
		GRAPHICS_CMD_LIST->ResourceBarrier(1, &toDepthRead);
	}

	hdrGroup.WaitResourceToTarget();
	hdrGroup.OMSetRenderTargetsReadOnlyDepth();

	RENDERMANAGER.SetGraphicsTable();
	mShader->Update();

	GRAPHICS_CMD_LIST->IASetVertexBuffers(0, 1, &mVBV);
	GRAPHICS_CMD_LIST->IASetIndexBuffer(&mIBV);
	GRAPHICS_CMD_LIST->DrawIndexedInstanced(indexCount, 1, 0, 0, 0);

	hdrGroup.WaitTargetToResource();

	if (depthResource != nullptr)
	{
		auto toDepthWrite = CD3DX12_RESOURCE_BARRIER::Transition(
			depthResource,
			D3D12_RESOURCE_STATE_DEPTH_READ | D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE,
			D3D12_RESOURCE_STATE_DEPTH_WRITE);
		GRAPHICS_CMD_LIST->ResourceBarrier(1, &toDepthWrite);
	}
}
