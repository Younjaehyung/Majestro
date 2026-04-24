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

void UIAudioVisualizerFeature::Initialize(World* world)
{
    mWorld     = world;
    mQuadMesh  = RESOURCEMANAGER.Get<Mesh>(L"UIQuad");
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

    // StartInstanceLocation = startInstance
    // → SV_InstanceID = startInstance + 0 ~ startInstance + barCount - 1
    // → UIInstances[SV_InstanceID] 가 UIInfo 버퍼의 바 데이터를 올바르게 인덱싱
    mQuadMesh->Render(barCount, 0, 0, startInstance);
}


void UIAudioVisualizerFeature::UpdateAudioVisualizer(float dt)
{
    if (mWorld->HasComponentPool<CircularVisualizerComponent>() == false)
        return;

    std::vector<float> spectrum;
    if (AUDIOMANAGER.GetSpectrumData(spectrum) == false)
        return;

    const int spectrumSize = static_cast<int>(spectrum.size());
    if (spectrumSize == 0)
        return;

    const float sampleRate = AUDIOMANAGER.GetSpectrumSampleRate();

    static std::mt19937 rng{ std::random_device{}() };
    std::uniform_int_distribution<int> countDist(2, 4);
    std::uniform_int_distribution<int> indexDist(0, CIRC_VIS_POINTS - 1);

    for (Entity entity : mWorld->GetEntitiesWithComponent<CircularVisualizerComponent>())
    {
        CircularVisualizerComponent* visualizer = mWorld->GetComponent<CircularVisualizerComponent>(entity);
        if (visualizer == nullptr || visualizer->isVisible == false)
            continue;

        float bands[kInternalBands] = {};
        for (int band = 0; band < kInternalBands; ++band)
        {
            auto [binStart, binEnd] = GetBinRange(band, kInternalBands, spectrumSize, sampleRate);

            float peak = 0.f;
            for (int bin = binStart; bin < binEnd; ++bin)
                peak = max(peak, spectrum[bin]);

            const float freqT = static_cast<float>(band) / static_cast<float>(kInternalBands - 1);
            const float eqGain = 0.5f + freqT * 3.5f;
            bands[band] = std::clamp(peak * visualizer->gain * eqGain, 0.6f, 1.f);
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

            const int count = countDist(rng);
            int slot = 0;
            for (auto& spike : visualizer->spikes)
            {
                if (slot >= count)
                    break;

                spike.pointIdx = indexDist(rng);
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
    AppendBarInstances(instances);

	uint32 regularCount = static_cast<uint32>(instances.size());

    uint32 barCount = static_cast<uint32>(instances.size()) - regularCount;

    if (barCount > 0)  Execute(regularCount, barCount);

}
