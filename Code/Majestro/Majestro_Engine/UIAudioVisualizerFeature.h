#pragma once
#include "World.h"
#include "UIRenderSystem.h"  // UIInstanceData
#include "UIFeature.h"

class Mesh;
class CircularVisualizerPass;

// AudioVisualizerPass
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
class UIAudioVisualizerFeature : public UIFeature
{
public:
    UIAudioVisualizerFeature() = default;
    ~UIAudioVisualizerFeature() = default;

    void Initialize(World* world);

    // CustomSpriteUpdate()에서 호출 — instances 벡터 뒤에 바 데이터 추가
    void AppendBarInstances(std::vector<UIInstanceData>& instances) const;

    void Update(float dt) override;

    // 바 인스턴스를 벡터 뒤에 추가
    void CustomSpriteRender(std::vector<UIInstanceData>& instances) override;


    void PostSpriteRender(std::vector<UIInstanceData>& instances) override;

    bool PostRendersInGroup(UIRenderGroup group) const override
    {
        return group == UIRenderGroup::Gameplay || group == UIRenderGroup::Dialogue;
    }

    // startInstance: UIInfo 버퍼에서 바 데이터가 시작되는 인덱스
    void Execute(uint32 startInstance, uint32 barCount);

private:

    void UpdateAudioVisualizer(float dt);
    std::pair<int, int> GetBinRange(int pointIdx, int totalPoints, int spectrumSize, float sampleRate) const;


private:
    static constexpr int kBassPoints = 6;
    static constexpr int kInternalBands = 32;

private:
    World*             mWorld     = nullptr;
    shared_ptr<Mesh>   mQuadMesh;

    // 원형 비주얼라이저 렌더 패스 (자체 버텍스 버퍼)
    shared_ptr<CircularVisualizerPass> mCircularPass;

    // 스펙트럼 폴링 버퍼
    std::vector<float> mSpectrum;

    // 빈 범위 캐시: 스펙트럼 크기가 바뀔 때만 재계산
    std::array<std::pair<int, int>, kInternalBands> mBinRanges{};
    int mCachedSpectrumSize = 0;

    // CustomSpriteRender(append)와 PostSpriteRender(draw) 사이에 전달되는 프레임 상태
    uint32 mBarStartInstance = 0;
    uint32 mBarCount         = 0;
};
