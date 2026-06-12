#include "pch.h"
#include "CircularVisualizerPass.h"
#include "Engine.h"
#include "ResourceManager.h"
#include "RenderManager.h"
#include "Shader.h"
#include "World.h"

// 막대당 4개 버텍스 (안쪽 좌/우, 바깥 좌/우)
static constexpr int kVertexCount = CIRC_VIS_POINTS * 4;

// 막대당 인덱스 6개 (삼각형 2개) — TRIANGLELIST.
static constexpr int kIndexCount  = CIRC_VIS_POINTS * 6;

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
    // 인덱스 패턴은 고정(정적).Upload 힙에 직접 씀
    std::vector<uint16_t> indices;
    indices.reserve(kIndexCount);

    for (int i = 0; i < CIRC_VIS_POINTS; i++)
    {
        uint16_t base = static_cast<uint16_t>(i * 4);
        // 막대 quad: v0(안쪽좌), v1(안쪽우), v2(바깥좌), v3(바깥우)
        // 삼각형 1: (v0, v2, v1), 삼각형 2: (v1, v2, v3) — CULL_NONE이므로 winding 무관
        indices.push_back(base + 0);
        indices.push_back(base + 2);
        indices.push_back(base + 1);

        indices.push_back(base + 1);
        indices.push_back(base + 2);
        indices.push_back(base + 3);
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

    // ── CPU에서 막대당 4개 버텍스 계산 ─────────────────────────────────
    // 막대 = 방사형 사각형(직사각 quad):
    //   - 각도는 barIndex / barCount 기준 균등 분배 (랜덤 없음)
    //   - 안쪽 끝은 baseRadius에 고정 (피벗 = 원 내부쪽 끝점)
    //   - 진폭이 커지면 바깥 방향(dir)으로만 길어짐 — 중심 방향 침범 없음
    //   - 두께(barWidth)는 반지름과 무관하게 일정 → 막대끼리 평행하게 분리됨
    //
    // 버텍스 배치 (막대 i, 기준 오프셋 = i * 4):
    //   [i*4+0] v0 = 안쪽 좌  (center + dir*innerR - perp*halfW)
    //   [i*4+1] v1 = 안쪽 우  (center + dir*innerR + perp*halfW)
    //   [i*4+2] v2 = 바깥 좌  (center + dir*outerR - perp*halfW)
    //   [i*4+3] v3 = 바깥 우  (center + dir*outerR + perp*halfW)
    const float halfW = vis->barWidth * 0.5f;

    for (int i = 0; i < CIRC_VIS_POINTS; i++)
    {
        const float angle = DirectX::XM_2PI * static_cast<float>(i)
                            / static_cast<float>(CIRC_VIS_POINTS);

        const float dirX = cosf(angle),  dirY = sinf(angle);   // 중심 → 바깥 방향
        const float perpX = -dirY,       perpY = dirX;          // 막대 두께 방향 (접선)

        // 진폭 → 길이: 무음이어도 minBarLength를 유지해 점선 형태의 링이 남는다
        const float amp = std::clamp(vis->waveAmplitudes[i], 0.f, 1.f);
        const float len = vis->minBarLength + amp * (vis->maxBarLength - vis->minBarLength);

        const float innerR = vis->baseRadius;        // 안쪽 끝 고정
        const float outerR = vis->baseRadius + len;  // 바깥쪽으로만 성장

        const int base = i * 4;

        // v0: 안쪽 좌
        mMappedVertices[base + 0].pos = Vec3(vis->center.x + dirX * innerR - perpX * halfW,
                                             vis->center.y + dirY * innerR - perpY * halfW, 0.f);
        mMappedVertices[base + 0].uv  = Vec2(amp, 0.f);

        // v1: 안쪽 우
        mMappedVertices[base + 1].pos = Vec3(vis->center.x + dirX * innerR + perpX * halfW,
                                             vis->center.y + dirY * innerR + perpY * halfW, 0.f);
        mMappedVertices[base + 1].uv  = Vec2(amp, 0.f);

        // v2: 바깥 좌
        mMappedVertices[base + 2].pos = Vec3(vis->center.x + dirX * outerR - perpX * halfW,
                                             vis->center.y + dirY * outerR - perpY * halfW, 0.f);
        mMappedVertices[base + 2].uv  = Vec2(amp, 0.f);

        // v3: 바깥 우
        mMappedVertices[base + 3].pos = Vec3(vis->center.x + dirX * outerR + perpX * halfW,
                                             vis->center.y + dirY * outerR + perpY * halfW, 0.f);
        mMappedVertices[base + 3].uv  = Vec2(amp, 0.f);
    }

    // ── PSO 바인딩 + 드로우 ─────────────────────────────────────────────
    // shader->Update() 내부에서 IASetPrimitiveTopology(TRIANGLELIST)도 호출됨
    shader->Update();
    GRAPHICS_CMD_LIST->IASetVertexBuffers(0, 1, &mVBV);
    GRAPHICS_CMD_LIST->IASetIndexBuffer(&mIBV);
    GRAPHICS_CMD_LIST->DrawIndexedInstanced(static_cast<uint32>(kIndexCount), 1, 0, 0, 0);
}
