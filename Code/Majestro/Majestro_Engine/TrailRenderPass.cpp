#include "pch.h"
#include "TrailRenderPass.h"

#include "DashSpeedLineComponent.h"
#include "Engine.h"
#include "Timer.h"
#include "RenderManager.h"
#include "ResourceManager.h"
#include "RenderTarget.h"
#include "Shader.h"
#include "Texture.h"
#include "TransformComponent.h"
#include "WeaponTrailComponent.h"
#include "World.h"

void TrailRenderPass::Initialize(World* world)
{
	mWorld = world;
	mShader = RESOURCEMANAGER.Get<Shader>(L"WeaponTrail");
}

void TrailRenderPass::Execute(const RenderContext& ctx)
{
	if (mWorld == nullptr || mShader == nullptr)
		return;

	std::vector<TrailRenderDesc> renderDescs;

	if (mWorld->HasComponentPool<WeaponTrailComponent>())
	{
		auto view = mWorld->View<WeaponTrailComponent>();
		for (Entity entity : view)
		{
			const WeaponTrailComponent* trail = mWorld->GetComponent<WeaponTrailComponent>(entity);
			if (trail == nullptr || trail->mIsActive == false)
				continue;

			TrailRenderDesc desc{};
			if (BuildDescFromWeaponTrail(*trail, desc))
				renderDescs.push_back(std::move(desc));
		}
	}

	if (mWorld->HasComponentPool<DashSpeedLineComponent>())
	{
		auto view = mWorld->View<DashSpeedLineComponent>();
		for (Entity entity : view)
		{
			const DashSpeedLineComponent* trail = mWorld->GetComponent<DashSpeedLineComponent>(entity);
			if (trail == nullptr || trail->mIsActive == false)
				continue;

			AppendDescsFromDashSpeedLine(*trail, renderDescs);
		}
	}

	if (renderDescs.empty())
		return;

	std::vector<Vertex> vertices;
	std::vector<uint32> indices;
	for (const TrailRenderDesc& desc : renderDescs)
	{
		BuildTrailMesh(desc, vertices, indices);
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

bool TrailRenderPass::BuildDescFromWeaponTrail(const WeaponTrailComponent& trail, TrailRenderDesc& out) const
{
	if (trail.mSamples.size() < 2)
		return false;

	// WeaponTrail 이동추적
	Matrix referenceFrame = Matrix::Identity;
	bool useLocalSamples = false;
	if (trail.mFollowOwnerMovement)	// 저장된 로컬 샘플을 현재 기준 월드행렬로 다시 변환
	{	// 과거에 찍힌 잔상도 매 프레임 플레이어의 현재 위치/회전을 따라 재배치
		const Entity referenceEntity = trail.mReferenceEntity.IsValid() ? trail.mReferenceEntity : trail.mSourceEntity;
		if (TransformComponent* refTransform = mWorld->GetComponent<TransformComponent>(referenceEntity))
		{
			referenceFrame = BuildWeaponTrailReferenceFrame(refTransform->mWorldMatrix, trail.mFollowReferenceRotation);
			useLocalSamples = true;
		}
	}


	out.Samples.reserve(trail.mSamples.size());
	for (const WeaponTrailSample& sample : trail.mSamples)
	{
		TrailRenderSample renderSample{};
		if (useLocalSamples)
		{
			// 저장 시 사용한 식의 역연산: 현재 playerWorld * localSample
			renderSample.LeftWorld = Vec3::Transform(sample.BaseLocal, referenceFrame);
			renderSample.RightWorld = Vec3::Transform(sample.TipLocal, referenceFrame);
		}
		else
		{
			renderSample.LeftWorld = sample.BaseWorld;
			renderSample.RightWorld = sample.TipWorld;
		}
		renderSample.Age = sample.Age;
		out.Samples.push_back(renderSample);
	}

	out.TextureName = trail.mTextureName;
	out.Style = static_cast<TrailRenderStyle>(trail.mVisualStyle);
	out.LayerCount = trail.mLayerCount;
	out.SmoothingSubdivisions = trail.mSmoothingSubdivisions;
	out.Lifetime = trail.mLifetime;
	out.BaseAlpha = trail.mBaseAlpha;
	out.Intensity = trail.mIntensity;
	out.UvTiling = trail.mUvTiling;
	out.WidthMultiplier = trail.mWidthMultiplier;
	out.TailWidthScale = trail.mTailWidthScale;
	out.MidWidthScale = trail.mMidWidthScale;
	out.HeadWidthScale = trail.mHeadWidthScale;
	out.LayerSpread = trail.mLayerSpread;
	out.CutStrength = trail.mSlashCutStrength;
	out.LineStrength = trail.mSlashLineStrength;
	out.EdgeBoost = trail.mSlashEdgeBoost;
	out.CoreBoost = trail.mSlashCoreBoost;
	out.TexScrollSpeed = trail.mSlashTexScrollSpeed;
	out.CoreColor = trail.mCoreColor;
	out.EdgeColor = trail.mEdgeColor;
	out.SubColor = trail.mSubColor;
	return true;
}

void TrailRenderPass::AppendDescsFromDashSpeedLine(const DashSpeedLineComponent& trail, std::vector<TrailRenderDesc>& out) const
{
	const size_t sampleCount = trail.mSamples.size();
	if (sampleCount < 2)
		return;

	const float lastIndex = static_cast<float>(sampleCount - 1);
	const uint32 lineCount = std::clamp(trail.mLineCount, 1u, 8u);

	for (uint32 line = 0; line < lineCount; ++line)
	{
		const float lineRate = lineCount > 1 ? static_cast<float>(line) / static_cast<float>(lineCount - 1) : 0.5f;
		const float lane = lineRate * 2.0f - 1.0f;
		const float laneOffset = lane * trail.mLineSpread;
		const float laneAlpha = 1.0f - std::abs(lane) * 0.25f;
		const float heightJitter = ((line % 2u) == 0u ? -0.25f : 0.35f) * trail.mLineWidth;

		TrailRenderDesc desc{};
		desc.Samples.reserve(sampleCount);
		for (size_t i = 0; i < sampleCount; ++i)
		{
			const DashSpeedLineSample& sample = trail.mSamples[i];
			const float pathRate = static_cast<float>(i) / lastIndex;
			const float centerFade = 1.0f - std::abs(pathRate * 2.0f - 1.0f) * 0.35f;
			const float halfWidth = trail.mLineWidth * max(centerFade, 0.25f) * 0.5f;
			const Vec3 center = sample.CenterWorld + sample.RightWorld * laneOffset + Vec3::Up * heightJitter;
			const Vec3 width = Vec3::Up * halfWidth;


			TrailRenderSample renderSample{};
			renderSample.LeftWorld = center - width;
			renderSample.RightWorld = center + width;
			renderSample.Age = sample.Age;
			desc.Samples.push_back(renderSample);
		}

		desc.TextureName = trail.mTextureName;
		desc.Style = TrailRenderStyle::Smoke;
		desc.LayerCount = 1;
		desc.SmoothingSubdivisions = 0;
		desc.Lifetime = trail.mLifetime;
		desc.BaseAlpha = trail.mBaseAlpha * laneAlpha;
		desc.Intensity = trail.mIntensity;
		desc.UvTiling = trail.mUvTiling;
		desc.WidthMultiplier = 1.0f;
		desc.TailWidthScale = 1.0f;
		desc.MidWidthScale = 1.0f;
		desc.HeadWidthScale = 1.0f;
		desc.LayerSpread = 0.0f;
		desc.CutStrength = trail.mBreakStrength;
		desc.LineStrength = trail.mLineStrength;
		desc.CoreColor = trail.mCoreColor;
		desc.EdgeColor = trail.mEdgeColor;
		desc.SubColor = trail.mSubColor;
		out.push_back(std::move(desc));
	}
}

Vec3 TrailRenderPass::CatmullRom(const Vec3& p0, const Vec3& p1, const Vec3& p2, const Vec3& p3, float t) const
{
	return Vec3::CatmullRom(p0, p1, p2, p3, t);
}

void TrailRenderPass::BuildSmoothedSamples(const std::vector<TrailRenderSample>& source, uint32 subdivisions, std::vector<TrailRenderSample>& out) const
{
	out.clear();
	if (source.size() < 2 || subdivisions == 0)
	{
		out = source;
		return;
	}

	const uint32 clampedSubdivisions = std::clamp(subdivisions, 0u, 4u);
	out.reserve(source.size() + (source.size() - 1) * clampedSubdivisions);

	for (size_t i = 0; i + 1 < source.size(); ++i)
	{
		const TrailRenderSample& p0 = source[i == 0 ? i : i - 1];
		const TrailRenderSample& p1 = source[i];
		const TrailRenderSample& p2 = source[i + 1];
		const TrailRenderSample& p3 = source[min(i + 2, source.size() - 1)];

		if (i == 0)
			out.push_back(p1);

		for (uint32 step = 1; step <= clampedSubdivisions; ++step)
		{
			const float t = static_cast<float>(step) / static_cast<float>(clampedSubdivisions + 1u);

			TrailRenderSample sample{};
			sample.LeftWorld = CatmullRom(p0.LeftWorld, p1.LeftWorld, p2.LeftWorld, p3.LeftWorld, t);
			sample.RightWorld = CatmullRom(p0.RightWorld, p1.RightWorld, p2.RightWorld, p3.RightWorld, t);
			sample.Age = p1.Age + (p2.Age - p1.Age) * t;
			out.push_back(sample);
		}

		out.push_back(p2);
	}
}

void TrailRenderPass::BuildTrailMesh(const TrailRenderDesc& desc, std::vector<Vertex>& vertices, std::vector<uint32>& indices) const
{
	const bool swordSlash = desc.Style == TrailRenderStyle::SwordSlash;

	// SwordSlash(검기)
	// 미설정(0)이면 최소 3단계를 강제
	const uint32 effectiveSubdivisions = swordSlash ? max(desc.SmoothingSubdivisions, 3u) : desc.SmoothingSubdivisions;

	std::vector<TrailRenderSample> smoothedSamples;
	const std::vector<TrailRenderSample>* meshSamples = &desc.Samples;
	if (effectiveSubdivisions > 0)
	{
		BuildSmoothedSamples(desc.Samples, effectiveSubdivisions, smoothedSamples);
		meshSamples = &smoothedSamples;
	}

	const size_t sampleCount = meshSamples->size();
	if (sampleCount < 2)
		return;

	const float lifetime = max(desc.Lifetime, 0.001f);
	const float lastIndex = static_cast<float>(sampleCount - 1);

	const float totalTime = TIMER.GetTotalTime();
	
	const bool hammerFlame = desc.Style == TrailRenderStyle::HammerFlame;
	const uint32 minLayerCount = hammerFlame ? 3u : 1u;
	const uint32 maxLayerCount = swordSlash ? 6u : 4u;
	const uint32 layerCount = std::clamp(desc.LayerCount, minLayerCount, maxLayerCount);

	float textureIndex = -1.0f;
	if (desc.TextureName.empty() == false)
	{
		if (shared_ptr<Texture> texture = RESOURCEMANAGER.Get<Texture>(desc.TextureName))
			textureIndex = static_cast<float>(texture->GetImageIndex());
	}

	vertices.reserve(vertices.size() + sampleCount * 2 * layerCount);
	indices.reserve(indices.size() + (sampleCount - 1) * 6 * layerCount);

	for (uint32 layer = 0; layer < layerCount; ++layer)
	{
		const uint32 baseVertex = static_cast<uint32>(vertices.size());
		const float layerRate = layerCount > 1 ? static_cast<float>(layer) / static_cast<float>(layerCount - 1) : 0.0f;
		const float layerOffsetScale = hammerFlame ? 0.55f : 1.0f;
		const float layerOffset = layer == 0 ? 0.0f : (layerRate * 2.0f - 1.0f) * desc.LayerSpread * layerOffsetScale;
		const float layerWidthMin = swordSlash ? 0.18f : (hammerFlame ? 0.34f : 0.42f);
		const float layerWidthMax = swordSlash ? 0.34f : (hammerFlame ? 0.58f : 0.72f);
		const float layerAlphaMin = swordSlash ? 0.45f : (hammerFlame ? 0.22f : 0.28f);
		const float layerAlphaMax = swordSlash ? 0.7f : (hammerFlame ? 0.42f : 0.52f);
		const float layerWidth = layer == 0 ? 1.0f : layerWidthMin + (layerWidthMax - layerWidthMin) * (1.0f - layerRate);
		const float layerAlpha = layer == 0 ? 1.0f : layerAlphaMin + (layerAlphaMax - layerAlphaMin) * (1.0f - layerRate);

		for (size_t i = 0; i < sampleCount; ++i)
		{
			const TrailRenderSample& sample = (*meshSamples)[i];
			const float pathRate = static_cast<float>(i) / lastIndex;
			const float ageRate = std::clamp(sample.Age / lifetime, 0.0f, 1.0f);
			const float alpha = desc.BaseAlpha * (1.0f - ageRate) * layerAlpha;

			float widthScale;
			if (swordSlash)
			{
				// 초승달(crescent) 폭 프로파일:
				// pathRate 0=꼬리, 1=머리
				// smoothstep 으로 폭 스케일을 곡선 보간(선형 각짐 제거).
				// sin(pathRate * pi) bulge(양끝0·중앙1)를 곱해 양끝을 한층 날카롭게
				const float midPoint = 0.5f;
				const float sTail = std::clamp(pathRate / midPoint, 0.0f, 1.0f);
				const float sHead = std::clamp((pathRate - midPoint) / (1.0f - midPoint), 0.0f, 1.0f);
				const float eTail = sTail * sTail * (3.0f - 2.0f * sTail); // smoothstep
				const float eHead = sHead * sHead * (3.0f - 2.0f * sHead); // smoothstep
				const float lowWidth = desc.TailWidthScale + (desc.MidWidthScale - desc.TailWidthScale) * eTail;
				const float highWidth = desc.MidWidthScale + (desc.HeadWidthScale - desc.MidWidthScale) * eHead;
				const float profile = (pathRate < midPoint) ? lowWidth : highWidth;
				const float bulge = sinf(pathRate * 3.14159265f); // 0(양끝)->1(중앙)
				widthScale = profile * (0.30f + 0.70f * bulge) * desc.WidthMultiplier * layerWidth;
			}
			else
			{
				// 기존 piecewise 보간(FlameRibbon/HammerFlame/SpeedLine 동작 유지).
				const float t0 = std::clamp(pathRate / 0.55f, 0.0f, 1.0f);
				const float t1 = std::clamp((pathRate - 0.55f) / 0.45f, 0.0f, 1.0f);
				const float ease0 = t0 * t0 * (3.0f - 2.0f * t0);
				const float ease1 = t1 * t1 * (3.0f - 2.0f * t1);
				const float lowWidth = desc.TailWidthScale + (desc.MidWidthScale - desc.TailWidthScale) * ease0;
				const float highWidth = desc.MidWidthScale + (desc.HeadWidthScale - desc.MidWidthScale) * ease1;
				widthScale = (pathRate < 0.55f ? lowWidth : highWidth) * desc.WidthMultiplier * layerWidth;
			}

			const Vec3 halfWidth = (sample.RightWorld - sample.LeftWorld) * 0.5f;
			const Vec3 layerShift = halfWidth * layerOffset;

			// SwordSlash(초승달):
			Vec3 leftPos;
			Vec3 rightPos;
			if (swordSlash)
			{
				const Vec3 fullWidth = sample.RightWorld - sample.LeftWorld;
				const Vec3 baseAnchor = sample.LeftWorld + layerShift;
				leftPos = baseAnchor;
				rightPos = baseAnchor + fullWidth * widthScale;
			}
			else
			{
				const Vec3 center = (sample.LeftWorld + sample.RightWorld) * 0.5f;
				const Vec3 scaledHalfWidth = halfWidth * widthScale;
				leftPos = center + layerShift - scaledHalfWidth;
				rightPos = center + layerShift + scaledHalfWidth;
			}
			
			const float vCoord = pathRate * desc.UvTiling - (swordSlash ? totalTime * desc.TexScrollSpeed : 0.0f);

			Vertex leftVertex{};
			leftVertex.pos = leftPos;
			leftVertex.uv = Vec2(0.0f, vCoord);
			leftVertex.tangent = Vec3(alpha, ageRate, desc.Intensity);
			leftVertex.weights = Vec4(textureIndex, static_cast<float>(layer), static_cast<float>(desc.Style), desc.CutStrength);

			Vertex rightVertex{};
			rightVertex.pos = rightPos;
			rightVertex.uv = Vec2(1.0f, vCoord);
			rightVertex.tangent = Vec3(alpha, ageRate, desc.Intensity);
			rightVertex.weights = Vec4(textureIndex, static_cast<float>(layer), static_cast<float>(desc.Style), desc.CutStrength);

			if (swordSlash)	// SwordSlash(검기)
			{
				const Vec3 edge = desc.EdgeColor * desc.EdgeBoost;
				const Vec3 core = desc.CoreColor * desc.CoreBoost;
				leftVertex.normal = edge;
				rightVertex.normal = edge;
				leftVertex.indices = Vec4(core.x, core.y, core.z, desc.LineStrength);
				rightVertex.indices = Vec4(core.x, core.y, core.z, desc.LineStrength);
			}
			else
			{
				// 기존 동작: 왼쪽=EdgeColor, 오른쪽=CoreColor 가 폭 방향으로 보간되고 SubColor 를 함께 전달.
				leftVertex.normal = desc.EdgeColor;
				rightVertex.normal = desc.CoreColor;
				leftVertex.indices = Vec4(desc.SubColor.x, desc.SubColor.y, desc.SubColor.z, desc.LineStrength);
				rightVertex.indices = Vec4(desc.SubColor.x, desc.SubColor.y, desc.SubColor.z, desc.LineStrength);
			}

			vertices.push_back(leftVertex);
			vertices.push_back(rightVertex);
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
}
