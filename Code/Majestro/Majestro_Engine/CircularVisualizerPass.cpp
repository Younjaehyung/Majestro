#include "pch.h"
#include "CircularVisualizerPass.h"
#include "Engine.h"
#include "ResourceManager.h"
#include "RenderManager.h"
#include "Shader.h"
#include "World.h"

// 빈당 4개 버텍스, 닫기 없음 (각 빈이 독립 quad)
static constexpr int kVertexCount = CIRC_VIS_POINTS * 4;

// 빈당 인덱스 5개: [v0, v2, v1, v3, 0xFFFF(cut)]
// 0xFFFF = TRIANGLESTRIP strip restart value (PSO IBStripCutValue_0xFFFF)
static constexpr int kIndexCount  = CIRC_VIS_POINTS * 5;

void CircularVisualizerPass::Initialize(World* world)
{
    mWorld = world;
    CreateVertexBuffer();
    CreateIndexBuffer();
}

void CircularVisualizerPass::CreateVertexBuffer()
{
    const uint32 bufferSize = static_cast<uint32>(kVertexCount * sizeof(Vertex));

    D3D12_HEAP_PROPERTIES heapProp = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD);
    D3D12_RESOURCE_DESC   desc     = CD3DX12_RESOURCE_DESC::Buffer(bufferSize);

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
    mVBV.StrideInBytes  = sizeof(Vertex);
    mVBV.SizeInBytes    = bufferSize;
}

void CircularVisualizerPass::CreateIndexBuffer()
{
    // 인덱스 패턴은 고정(정적). N=128이면 128*5*2 = 1280 bytes로 매우 작으므로
    // 복잡한 Default힙 업로드 없이 Upload 힙에 직접 씀 (성능 영향 미미).
    std::vector<uint16_t> indices;
    indices.reserve(kIndexCount);

    for (int i = 0; i < CIRC_VIS_POINTS; i++)
    {
        uint16_t base = static_cast<uint16_t>(i * 4);
        // Triangle Strip quad: [v0(안쪽좌), v2(바깥좌), v1(안쪽우), v3(바깥우)]
        // → 삼각형 1: (v0, v2, v1), 삼각형 2: (v2, v1, v3) — CULL_NONE이므로 winding 무관
        indices.push_back(base + 0);  // v0 = 안쪽 좌
        indices.push_back(base + 2);  // v2 = 바깥 좌
        indices.push_back(base + 1);  // v1 = 안쪽 우
        indices.push_back(base + 3);  // v3 = 바깥 우
        indices.push_back(0xFFFF);    // strip cut: 다음 빈의 quad와 연결 차단
    }

    const uint32 bufferSize = static_cast<uint32>(kIndexCount * sizeof(uint16_t));

    D3D12_HEAP_PROPERTIES heapProp = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD);
    D3D12_RESOURCE_DESC   desc     = CD3DX12_RESOURCE_DESC::Buffer(bufferSize);

    DEVICE->CreateCommittedResource(
        &heapProp,
        D3D12_HEAP_FLAG_NONE,
        &desc,
        D3D12_RESOURCE_STATE_GENERIC_READ,
        nullptr,
        IID_PPV_ARGS(&mIndexBuffer));

    void* mapped = nullptr;
    CD3DX12_RANGE readRange(0, 0);
    mIndexBuffer->Map(0, &readRange, &mapped);
    memcpy(mapped, indices.data(), bufferSize);
    mIndexBuffer->Unmap(0, nullptr);

    mIBV.BufferLocation = mIndexBuffer->GetGPUVirtualAddress();
    mIBV.Format         = DXGI_FORMAT_R16_UINT;
    mIBV.SizeInBytes    = bufferSize;
}

void CircularVisualizerPass::Execute()
{
    if (!mWorld || !mMappedVertices)
        return;
    if (!mWorld->HasComponentPool<CircularVisualizerComponent>())
        return;

    auto entities = mWorld->GetEntitiesWithComponent<CircularVisualizerComponent>();
    if (entities.empty())
        return;

    auto* vis = mWorld->GetComponent<CircularVisualizerComponent>(entities[0]);
    if (!vis || !vis->isVisible)
        return;

    auto* shader = RESOURCEMANAGER.Get<Shader>(L"CircularVisualizer").get();
    if (!shader)
        return;

    // ── 각도 파라미터 ──────────────────────────────────────────────────────
    const float binAngle  = DirectX::XM_2PI / static_cast<float>(CIRC_VIS_POINTS);
    const float gapAngle  = binAngle * vis->binGapRatio;
    const float fillAngle = binAngle - gapAngle;

    // ── 두께 파라미터 ──────────────────────────────────────────────────────
    const float amplifyScale = vis->maxAmplitude;
    const float minThickness = vis->baseRadius * 0.01f;  // 무음 시 최소 선 두께

    // ── CPU에서 빈당 4개 버텍스 계산 ─────────────────────────────────────
    // 버텍스 배치 (빈 i, 기준 오프셋 = i * 4):
    //   [i*4+0] v0 = (angle_left,  innerR)  안쪽 좌
    //   [i*4+1] v1 = (angle_right, innerR)  안쪽 우
    //   [i*4+2] v2 = (angle_left,  outerR)  바깥 좌
    //   [i*4+3] v3 = (angle_right, outerR)  바깥 우
    for (int i = 0; i < CIRC_VIS_POINTS; i++)
    {
        float angleCenter = binAngle * static_cast<float>(i);
        float angleLeft   = angleCenter - fillAngle * 0.5f;
        float angleRight  = angleCenter + fillAngle * 0.5f;

        float amp         = vis->waveAmplitudes[i];
        float halfThick   = max(amp * amplifyScale, minThickness);

        float outerR      = vis->baseRadius + halfThick * 0.7f;
        float innerR      = vis->baseRadius - halfThick * 0.3f;
        innerR            = max(innerR, vis->baseRadius * 0.85f);

        float cosL = cosf(angleLeft),  sinL = sinf(angleLeft);
        float cosR = cosf(angleRight), sinR = sinf(angleRight);

        int base = i * 4;

        // v0: 안쪽 좌
        mMappedVertices[base + 0].pos = Vec3(vis->center.x + cosL * innerR,
                                             vis->center.y + sinL * innerR, 0.f);
        mMappedVertices[base + 0].uv  = Vec2(amp, 0.f);

        // v1: 안쪽 우
        mMappedVertices[base + 1].pos = Vec3(vis->center.x + cosR * innerR,
                                             vis->center.y + sinR * innerR, 0.f);
        mMappedVertices[base + 1].uv  = Vec2(amp, 0.f);

        // v2: 바깥 좌
        mMappedVertices[base + 2].pos = Vec3(vis->center.x + cosL * outerR,
                                             vis->center.y + sinL * outerR, 0.f);
        mMappedVertices[base + 2].uv  = Vec2(amp, 0.f);

        // v3: 바깥 우
        mMappedVertices[base + 3].pos = Vec3(vis->center.x + cosR * outerR,
                                             vis->center.y + sinR * outerR, 0.f);
        mMappedVertices[base + 3].uv  = Vec2(amp, 0.f);
    }

    // ── PSO 바인딩 + 드로우 ─────────────────────────────────────────────
    // shader->Update() 내부에서 IASetPrimitiveTopology(TRIANGLESTRIP)도 호출됨
    shader->Update();
    GRAPHICS_CMD_LIST->IASetVertexBuffers(0, 1, &mVBV);
    GRAPHICS_CMD_LIST->IASetIndexBuffer(&mIBV);
    GRAPHICS_CMD_LIST->DrawIndexedInstanced(static_cast<uint32>(kIndexCount), 1, 0, 0, 0);
}
