#pragma once
#include <d3d12.h>
#include <wrl.h>
#include <vector>
#include <string>
#include <map>
#include <cstdint>

// DX12 타임스탬프 쿼리 기반 GPU 구간 계측기. 성능 로그가 꺼져 있으면 아무 일도 하지 않는다.
//   GPU_MARKER(GpuScopedMarker)가 Begin/EndScope 를 호출하므로 마커가 있는 패스는 자동 계측된다.
//   "Frame" 구간은 BeginFrame~EndFrame 전체(=그래픽 큐 기준 GPU 프레임 벽시간)이다.
//   결과는 FRAME_COUNT 프레임 뒤 같은 슬롯을 다시 쓸 때 읽는다(WaitForFrame 이 완료를 보장).
class GpuProfiler
{
public:
    static GpuProfiler& Get();

    void BeginFrame(ID3D12GraphicsCommandList* cl, uint32_t frameIndex);   // RenderBegin 직후
    void EndFrame(ID3D12GraphicsCommandList* cl, uint32_t frameIndex);     // 커맨드 리스트 Close 직전

    void BeginScope(ID3D12GraphicsCommandList* cl, const wchar_t* name);
    void EndScope(ID3D12GraphicsCommandList* cl);

    // 마지막 Flush 이후 누적 평균을 "[GPU] ..." 한 줄로 만들고 누적값을 비운다.
    std::wstring FlushLine();

private:
    void EnsureInit();
    int  Stamp(ID3D12GraphicsCommandList* cl);
    void Accumulate(uint32_t slot);

    static constexpr uint32_t MAX_TIMESTAMPS = 256;  // 슬롯당(=128 scope)
    static constexpr uint32_t FRAME_COUNT    = 3;    // == FRAMEGROUP_COUNT

    struct ScopeRec { std::wstring name; uint32_t beginIdx; uint32_t endIdx; };

    bool mInitialized = false;
    Microsoft::WRL::ComPtr<ID3D12QueryHeap> mHeap;
    Microsoft::WRL::ComPtr<ID3D12Resource>  mReadback;
    uint64_t* mMapped = nullptr;
    uint64_t  mFreq   = 1;

    std::vector<ScopeRec> mScopes[FRAME_COUNT];
    uint32_t              mCount[FRAME_COUNT] = {};
    std::vector<uint32_t> mStack;
    uint32_t              mCurFrame  = 0;
    bool                  mFrameOpen = false;

    std::vector<std::pair<std::wstring, double>> mAccum;   // 등장 순서 유지
    uint32_t mAccumFrames = 0;
};
