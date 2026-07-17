#include "pch.h"
#include "UIAudioVisualizerFeature.h"
#include "AudioVisualizerComponent.h"
#include "Engine.h"
#include "ResourceManager.h"
#include "RenderManager.h"
#include "AudioManager.h"
#include "Mesh.h"
#include "Shader.h"
#include "World.h"
#include "CircularVisualizerComponent.h"
#include "CircularVisualizerPass.h"
#include "MathUtils.h"

void UIAudioVisualizerFeature::Initialize(World* world)
{
    mWorld     = world;
    mQuadMesh  = RESOURCEMANAGER.Get<Mesh>(L"UIQuad");


    mCircularPass = std::make_shared<CircularVisualizerPass>();
    mCircularPass->Initialize(world);
}

void UIAudioVisualizerFeature::AppendBarInstances(std::vector<UIInstanceData>& instances) const
{
    if (!mWorld || !mWorld->HasComponentPool<AudioVisualizerComponent>())
        return;

    auto entities = mWorld->GetEntitiesWithComponent<AudioVisualizerComponent>();
    if (entities.empty())
        return;

    // 첫 번째 컴포넌트만 사용 (복수 비주얼라이저는 현재 미지원)
    auto* vis = mWorld->GetComponent<AudioVisualizerComponent>(entities[0]);
    if (!vis || !vis->isVisible)
        return;

    // 전체 바 영역 좌측 끝 픽셀 X = 중심 - (전체 폭 / 2)
    float totalWidth = static_cast<float>(VISUALIZER_BAR_COUNT) * (vis->barWidth + vis->barSpacing)
                       - vis->barSpacing;
    float startX     = vis->basePosition.x - totalWidth * 0.5f + vis->barWidth * 0.5f;

    for (int i = 0; i < VISUALIZER_BAR_COUNT; i++)
    {
        float barH = vis->barHeights[i] * vis->maxBarHeight;
        // 최솟값: 눈에 보이지 않을 정도로 작지만 0이면 NDC 변환이 이상해지므로 1px 유지
        barH = max(barH, 1.f);

        UIInstanceData inst{};

        // Position: 바 하단 중앙 픽셀 좌표
        inst.Position.x = startX + static_cast<float>(i) * (vis->barWidth + vis->barSpacing);
        inst.Position.y = vis->basePosition.y;

        // Size: 픽셀 크기
        inst.Size.x = vis->barWidth;
        inst.Size.y = barH;

        // Pivot: (0.5, 1) — 수평 중앙, 하단 기준
        // UI_VS.hlsl: local = pos.xy - pivot → 바 아래에서 위로 성장
        inst.Pivot.x = 0.5f;
        inst.Pivot.y = 1.0f;

        // MaterialIndex: 바 인덱스 (0~63) — visualizer_PS.hlsl에서 색상 그라데이션 계산에 사용
        inst.MaterialIndex = static_cast<uint32>(i);

        // ZOrder: 정규화된 바 높이 (0~1) — PS에서 글로우 강도 계산에 사용
        // UI 깊이 정렬과 충돌하지 않도록 0에 가까운 값 사용 (NO_DEPTH_TEST이므로 실제 영향 없음)
        inst.ZOrder = vis->barHeights[i];

        instances.push_back(inst);
    }
}

void UIAudioVisualizerFeature::Update(float dt)
{
    UpdateAudioVisualizer(dt);
}

void UIAudioVisualizerFeature::Execute(uint32 startInstance, uint32 barCount)
{
    if (!mQuadMesh || barCount == 0)
        return;

    auto* shader = RESOURCEMANAGER.Get<Shader>(L"AudioVisualizer").get();
    if (!shader)
        return;

    shader->Update();  // PSO 설정


    GRAPHICS_CMD_LIST->SetGraphicsRoot32BitConstants(0, 1, &startInstance, 0);
    mQuadMesh->Render(barCount, 0, 0, 0);
}


void UIAudioVisualizerFeature::UpdateAudioVisualizer(float dt)
{
    if (mWorld->HasComponentPool<CircularVisualizerComponent>() == false)
        return;

    // 보이는 비주얼라이저가 없으면 폴링하지 않는다 
    auto entities = mWorld->GetEntitiesWithComponent<CircularVisualizerComponent>();
    bool anyVisible = false;
    for (Entity entity : entities)
    {
        CircularVisualizerComponent* vis = mWorld->GetComponent<CircularVisualizerComponent>(entity);
        if (vis != nullptr && vis->isVisible)
        {
            anyVisible = true;
            break;
        }
    }
    if (anyVisible == false)
        return;

    // 멤버 버퍼 재사용 — 매 프레임 힙 할당 방지
    if (AUDIOMANAGER.GetSpectrumData(mSpectrum) == false)
        return;

    const int spectrumSize = static_cast<int>(mSpectrum.size());
    if (spectrumSize == 0)
        return;

    const float sampleRate = AUDIOMANAGER.GetSpectrumSampleRate();

    // 빈 범위는 스펙트럼 크기에만 의존하므로 크기가 바뀔 때만 재계산
    if (spectrumSize != mCachedSpectrumSize)
    {
        for (int band = 0; band < kInternalBands; ++band)
            mBinRanges[band] = GetBinRange(band, kInternalBands, spectrumSize, sampleRate);
        mCachedSpectrumSize = spectrumSize;
    }

    for (Entity entity : entities)
    {
        CircularVisualizerComponent* visualizer = mWorld->GetComponent<CircularVisualizerComponent>(entity);
        if (visualizer == nullptr || visualizer->isVisible == false)
            continue;

        float bands[kInternalBands] = {};
        for (int band = 0; band < kInternalBands; ++band)
        {
            auto [binStart, binEnd] = mBinRanges[band];

            float peak = 0.f;
            for (int bin = binStart; bin < binEnd; ++bin)
                peak = max(peak, mSpectrum[bin]);

            const float freqT = static_cast<float>(band) / static_cast<float>(kInternalBands - 1);
            const float eqGain = 0.5f + freqT * 3.5f;
            // 하한 0: 무음 구간은 진폭 0이어야 막대가 minBarLength(점선 링)로 가라앉는다
            // (기존 0.6 바닥값은 옛 리본 디자인용 — 막대 스타일에서는 부적합)
            bands[band] = std::clamp(peak * visualizer->gain * eqGain, 0.f, 1.f);
        }

        float bassEnergy = 0.f;
        for (int i = 0; i < CIRC_VIS_POINTS; ++i)
        {
            const float t = static_cast<float>(i) / static_cast<float>(CIRC_VIS_POINTS - 1) * static_cast<float>(kInternalBands - 1);
            const int lo = static_cast<int>(t);
            const int hi = min(lo + 1, kInternalBands - 1);
            const float frac = t - static_cast<float>(lo);

            const float target = bands[lo] * (1.f - frac) + bands[hi] * frac;
            float& current = visualizer->waveAmplitudes[i];

            const float speed = (target > current) ? visualizer->riseSmooth : visualizer->fallSmooth;
            current += (target - current) * speed * dt;
            current = std::clamp(current, 0.f, 1.f);

            if (i < kBassPoints)
                bassEnergy += current;
        }
        bassEnergy /= static_cast<float>(kBassPoints);

        // 스파이크는 선택 기능 — 미니멀 스타일(균등·FFT 기반)에서는 기본 비활성.
        // 활성 시 저음 비트마다 랜덤 막대가 순간 돌출한다.
        if (visualizer->useSpikes == false)
            continue;

        visualizer->cooldownTimer -= dt;
        for (auto& spike : visualizer->spikes)
        {
            if (spike.pointIdx < 0)
                continue;

            spike.timer -= dt;
            if (spike.timer <= 0.f)
            {
                spike.pointIdx = -1;
                spike.strength = 0.f;
            }
            else
            {
                spike.strength = spike.timer / visualizer->spikeDuration;
            }
        }

        if (bassEnergy >= visualizer->spikeThreshold && visualizer->cooldownTimer <= 0.f)
        {
            visualizer->cooldownTimer = visualizer->spikeCooldown;

            const int count = MathUtils::RandomInt(2, 4);
            int slot = 0;
            for (auto& spike : visualizer->spikes)
            {
                if (slot >= count)
                    break;

                spike.pointIdx = MathUtils::RandomInt(0, CIRC_VIS_POINTS - 1);
                spike.strength = 1.f;
                spike.timer = visualizer->spikeDuration;
                ++slot;
            }
        }

        for (const auto& spike : visualizer->spikes)
        {
            if (spike.pointIdx < 0)
                continue;

            float& amplitude = visualizer->waveAmplitudes[spike.pointIdx];
            const float spikeAmplitude = spike.strength * visualizer->spikeMultiplier;
            amplitude = max(amplitude, spikeAmplitude);
        }
    }
}

std::pair<int, int> UIAudioVisualizerFeature::GetBinRange(
    int pointIdx, int totalPoints, int spectrumSize, float sampleRate) const
{
    constexpr float kMinHz = 20.f;
    constexpr float kMaxHz = 16000.f;
    const float logMin = std::log2f(kMinHz);
    const float logMax = std::log2f(kMaxHz);

    const float t0 = static_cast<float>(pointIdx) / static_cast<float>(totalPoints);
    const float t1 = static_cast<float>(pointIdx + 1) / static_cast<float>(totalPoints);

    const float hz0 = std::pow(2.f, logMin + t0 * (logMax - logMin));
    const float hz1 = std::pow(2.f, logMin + t1 * (logMax - logMin));

    const float hzPerBin = (sampleRate * 0.5f) / static_cast<float>(spectrumSize);
    int binStart = static_cast<int>(hz0 / hzPerBin);
    int binEnd = static_cast<int>(hz1 / hzPerBin);

    binStart = std::clamp(binStart, 0, spectrumSize - 1);
    binEnd = std::clamp(binEnd + 1, binStart + 1, spectrumSize);
    return { binStart, binEnd };
}

void UIAudioVisualizerFeature::CustomSpriteRender(std::vector<UIInstanceData>& instances)
{
    // 바 데이터를 인스턴스 벡터 뒤에 추가만 한다.
    // 시작 오프셋/개수를 기억해 두고, 실제 드로우는 업로드 이후인 PostSpriteRender에서 수행.
    mBarStartInstance = static_cast<uint32>(instances.size());
    AppendBarInstances(instances);
    mBarCount = static_cast<uint32>(instances.size()) - mBarStartInstance;
}

void UIAudioVisualizerFeature::PostSpriteRender(std::vector<UIInstanceData>& /*instances*/)
{
    // UploadInstanceBuffer() 이후 호출되므로 UIInfo 버퍼에 바 데이터가 올라가 있다
    if (mBarCount > 0)
        Execute(mBarStartInstance, mBarCount);
    mBarCount = 0;

    // 원형 비주얼라이저 — 자체 버텍스 버퍼 드로우 (컴포넌트 없으면 내부에서 early-out)
    if (mCircularPass)
        mCircularPass->Execute();
}
