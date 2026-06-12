#pragma once
#include "World.h"
#include "CircularVisualizerComponent.h"

// CircularVisualizerPass
// 원 둘레에 균등 배치된 방사형 막대들로 이루어진 원형 오디오 비주얼라이저 렌더링.
//
// 렌더링 구조:
//   막대당 4개 버텍스: v0(안쪽좌), v1(안쪽우), v2(바깥좌), v3(바깥우)
//   인덱스 (막대당 6개): 삼각형 2개 — TRIANGLELIST (strip cut PSO 설정 불필요)
//    각 막대가 독립된 직사각 quad로 렌더링
//
// 막대 계산 (Execute):
//   angle  = 2*pi * i / N            — barIndex/barCount 기준 균등 분배
//   len    = minBarLength + amp * (maxBarLength - minBarLength)
//   innerR = baseRadius (고정)     — 막대 안쪽 끝 = 피벗
//   outerR = baseRadius + len      — 바깥 방향으로만 성장
//   두께   = barWidth (반지름과 무관하게 일정)
//
// 버텍스 레이아웃 (표준 Vertex 구조체 재사용):
//   pos.xy  = 화면 픽셀 좌표
//   pos.z   = 0
//   uv.x    = intensity (진폭, PS에서 알파 계산)
//   uv.y    = 미사용 (0)
//
// 버퍼 크기:
//   버텍스 버퍼: N * 4  (HEAP_TYPE_UPLOAD, 영구 맵핑)
//   인덱스 버퍼: N * 6 × sizeof(uint16)  (HEAP_TYPE_UPLOAD, 초기화 시 1회 기록)
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
