#pragma once
#include "World.h"
#include "CircularVisualizerComponent.h"

// CircularVisualizerPass
// 주파수 빈 하나가 독립된 뾰족한 돌기(spike)로 보이는 얇고 날카로운 파동 띠 렌더링.
//
// 렌더링 구조:
//   빈(bin)당 4개 버텍스: v0(안쪽좌), v1(안쪽우), v2(바깥좌), v3(바깥우)
//   인덱스 (빈당 5개): [v0, v2, v1, v3, 0xFFFF(cut)]
//   PSO: TRIANGLESTRIP + IBStripCutValue_0xFFFF → 빈 사이를 strip cut으로 분리
//   → 각 빈이 독립된 quad로 렌더링됨 (인접 빈이 연결되지 않음)
//
// 두께 계산:
//   halfThickness = max(amp * maxAmplitude, baseRadius * 0.01)
//   outerR = baseRadius + halfThickness * 0.7
//   innerR = baseRadius - halfThickness * 0.3  (최소 baseRadius * 0.85)
//
// 각도 계산:
//   binAngle   = 2π / N
//   gapAngle   = binAngle * binGapRatio
//   fillAngle  = binAngle - gapAngle
//   angle_left  = center - fillAngle * 0.5
//   angle_right = center + fillAngle * 0.5
//
// 버텍스 레이아웃 (표준 Vertex 구조체 재사용):
//   pos.xy  = 화면 픽셀 좌표
//   pos.z   = 0
//   uv.x    = intensity (진폭, PS에서 edge fade 계산)
//   uv.y    = 미사용 (0)
//
// 버퍼 크기:
//   버텍스 버퍼: N * 4  (HEAP_TYPE_UPLOAD, 영구 맵핑)
//   인덱스 버퍼: N * 5 × sizeof(uint16)  (HEAP_TYPE_DEFAULT, 초기화 시 1회 업로드)
class CircularVisualizerPass
{
public:
    CircularVisualizerPass()  = default;
    ~CircularVisualizerPass() = default;

    void Initialize(World* world);
    void Execute();

private:
    void CreateVertexBuffer();
    void CreateIndexBuffer();

private:
    World*                   mWorld          = nullptr;

    // 버텍스 버퍼 (Upload 힙, 영구 맵핑)
    ComPtr<ID3D12Resource>   mVertexBuffer;
    D3D12_VERTEX_BUFFER_VIEW mVBV            = {};
    Vertex*                  mMappedVertices = nullptr;

    // 인덱스 버퍼 (Default 힙, 초기화 시 1회 업로드)
    ComPtr<ID3D12Resource>   mIndexBuffer;
    D3D12_INDEX_BUFFER_VIEW  mIBV            = {};
};
