#pragma once
#include "World.h"
#include "UIRenderSystem.h"  // UIInstanceData

class Mesh;

// AudioVisualizerPass
// UIEffectPass와 동일한 헬퍼 패턴.
// UIRenderSystem이 소유하며 CustomSpriteUpdate() 흐름에 통합된다.
//
// 동작 방식:
//   1. AppendBarInstances() — AudioVisualizerComponent에서 바 데이터를 읽어
//      UIInstanceData 형식으로 인스턴스 벡터 뒤에 추가한다.
//   2. Execute()            — AudioVisualizer 셰이더로 바를 인스턴싱 렌더링한다.
//
// UIInfo 버퍼 레이아웃:
//   [0 .. regularCount-1]          = 일반 UI 스프라이트
//   [regularCount .. regularCount+barCount-1] = 비주얼라이저 바
//
// 셰이더는 UI_VS.hlsl을 그대로 재사용하고 visualizer_PS.hlsl만 교체한다.
// DrawIndexedInstanced의 StartInstanceLocation으로 SV_InstanceID가 오프셋되므로
// UIInstances[SV_InstanceID]는 각 파트에서 올바른 데이터를 읽는다.
class AudioVisualizerPass
{
public:
    AudioVisualizerPass() = default;
    ~AudioVisualizerPass() = default;

    void Initialize(World* world);

    // CustomSpriteUpdate()에서 호출 — instances 벡터 뒤에 바 데이터 추가
    void AppendBarInstances(std::vector<UIInstanceData>& instances) const;

    // UploadInstanceBuffer() 및 일반 UI 렌더링 이후 호출
    // startInstance: UIInfo 버퍼에서 바 데이터가 시작되는 인덱스
    void Execute(uint32 startInstance, uint32 barCount);

private:
    World*             mWorld     = nullptr;
    shared_ptr<Mesh>   mQuadMesh;
};
