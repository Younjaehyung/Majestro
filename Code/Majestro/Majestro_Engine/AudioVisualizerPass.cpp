#include "pch.h"
#include "AudioVisualizerPass.h"
#include "AudioVisualizerComponent.h"
#include "Engine.h"
#include "ResourceManager.h"
#include "RenderManager.h"
#include "Mesh.h"
#include "Shader.h"
#include "World.h"

void AudioVisualizerPass::Initialize(World* world)
{
    mWorld     = world;
    mQuadMesh  = RESOURCEMANAGER.Get<Mesh>(L"UIQuad");
}

void AudioVisualizerPass::AppendBarInstances(std::vector<UIInstanceData>& instances) const
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

void AudioVisualizerPass::Execute(uint32 startInstance, uint32 barCount)
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
