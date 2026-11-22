#include "pch.h"
#include "GpuProfiler.h"
#include "Engine.h"
#include "RenderManager.h"
#include "EngineLog.h"

GpuProfiler& GpuProfiler::Get()
{
    static GpuProfiler sInstance;
    return sInstance;
}

void GpuProfiler::EnsureInit()
{
    if (mInitialized)
        return;

    auto device = DEVICE;
    if (device == nullptr)
        return;

    D3D12_QUERY_HEAP_DESC qd{};
    qd.Type  = D3D12_QUERY_HEAP_TYPE_TIMESTAMP;
    qd.Count = MAX_TIMESTAMPS * FRAME_COUNT;
    if (FAILED(device->CreateQueryHeap(&qd, IID_PPV_ARGS(&mHeap))))
        return;

    const uint64_t bytes = static_cast<uint64_t>(MAX_TIMESTAMPS) * FRAME_COUNT * sizeof(uint64_t);
    CD3DX12_HEAP_PROPERTIES heapProp(D3D12_HEAP_TYPE_READBACK);
    CD3DX12_RESOURCE_DESC   resDesc = CD3DX12_RESOURCE_DESC::Buffer(bytes);
    if (FAILED(device->CreateCommittedResource(
            &heapProp, D3D12_HEAP_FLAG_NONE, &resDesc,
            D3D12_RESOURCE_STATE_COPY_DEST, nullptr, IID_PPV_ARGS(&mReadback))))
        return;

    mReadback->Map(0, nullptr, reinterpret_cast<void**>(&mMapped));

    GRAPHICS_CMD_QUEUE->GetCommandQueue()->GetTimestampFrequency(&mFreq);
    if (mFreq == 0)
        mFreq = 1;

    mInitialized = true;
}

void GpuProfiler::BeginFrame(ID3D12GraphicsCommandList* cl, uint32_t frameIndex)
{
    // 성능 로그가 꺼져 있으면 프레임을 열지 않는다. 이후 Begin/EndScope·EndFrame 은 전부 무동작이 된다.
    if (!EngineLog::Enabled(EngineLog::Domain::FramePerformance))
        return;

    EnsureInit();
    if (!mInitialized || cl == nullptr)
        return;

    const uint32_t f = frameIndex % FRAME_COUNT;

    // 이 슬롯에는 FRAME_COUNT 프레임 전 기록이 있고 GPU 완료가 보장된다.
    Accumulate(f);

    mScopes[f].clear();
    mCount[f] = 0;
    mStack.clear();
    mCurFrame  = f;
    mFrameOpen = true;

    BeginScope(cl, L"Frame");
}

int GpuProfiler::Stamp(ID3D12GraphicsCommandList* cl)
{
    const uint32_t f = mCurFrame;
    if (mCount[f] >= MAX_TIMESTAMPS)
        return -1;
    const uint32_t idx = f * MAX_TIMESTAMPS + mCount[f]++;
    cl->EndQuery(mHeap.Get(), D3D12_QUERY_TYPE_TIMESTAMP, idx);
    return static_cast<int>(idx);
}

void GpuProfiler::BeginScope(ID3D12GraphicsCommandList* cl, const wchar_t* name)
{
    if (!mFrameOpen || cl == nullptr)
        return;

    const int idx = Stamp(cl);
    if (idx < 0)
        return;

    const uint32_t f = mCurFrame;
    mStack.push_back(static_cast<uint32_t>(mScopes[f].size()));
    mScopes[f].push_back({ name, static_cast<uint32_t>(idx), static_cast<uint32_t>(idx) });
}

void GpuProfiler::EndScope(ID3D12GraphicsCommandList* cl)
{
    if (!mFrameOpen || cl == nullptr || mStack.empty())
        return;

    const uint32_t scopeArrayIdx = mStack.back();
    mStack.pop_back();

    const int idx = Stamp(cl);
    if (idx < 0)
        return;

    mScopes[mCurFrame][scopeArrayIdx].endIdx = static_cast<uint32_t>(idx);
}

void GpuProfiler::EndFrame(ID3D12GraphicsCommandList* cl, uint32_t frameIndex)
{
    if (!mFrameOpen || cl == nullptr)
    {
        mFrameOpen = false;
        return;
    }

    // 닫히지 않은 구간("Frame" 포함)을 모두 닫는다.
    while (!mStack.empty())
        EndScope(cl);

    const uint32_t f   = frameIndex % FRAME_COUNT;
    const uint32_t cnt = mCount[f];
    if (cnt > 0)
    {
        cl->ResolveQueryData(
            mHeap.Get(), D3D12_QUERY_TYPE_TIMESTAMP,
            f * MAX_TIMESTAMPS, cnt,
            mReadback.Get(), static_cast<uint64_t>(f) * MAX_TIMESTAMPS * sizeof(uint64_t));
    }

    mFrameOpen = false;
}

void GpuProfiler::Accumulate(uint32_t f)
{
    if (mScopes[f].empty() || mMapped == nullptr)
        return;

    for (const ScopeRec& rec : mScopes[f])
    {
        const uint64_t t0 = mMapped[rec.beginIdx];
        const uint64_t t1 = mMapped[rec.endIdx];
        const double ms = (t1 > t0 && t0 != 0)
            ? static_cast<double>(t1 - t0) / static_cast<double>(mFreq) * 1000.0
            : 0.0;

        auto it = std::find_if(mAccum.begin(), mAccum.end(),
            [&rec](const auto& entry) { return entry.first == rec.name; });
        if (it == mAccum.end())
            mAccum.emplace_back(rec.name, ms);
        else
            it->second += ms;
    }
    ++mAccumFrames;
}

std::wstring GpuProfiler::FlushLine()
{
    std::wstring line = L"[GPU] avg ms";
    if (mAccumFrames == 0)
        return line + L" | (no data)\n";

    wchar_t buf[96]{};
    for (const auto& [name, totalMs] : mAccum)
    {
        swprintf_s(buf, L" | %ls %.3f", name.c_str(), totalMs / mAccumFrames);
        line += buf;
    }
    line += L"\n";

    mAccum.clear();
    mAccumFrames = 0;
    return line;
}
