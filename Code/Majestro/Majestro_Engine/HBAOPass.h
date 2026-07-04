#pragma once
#include "RenderPass.h"
#include "Texture.h"


//
// 파이프라인 역할:
//   GBufferPass 직후, LightsPass 직전에 실행
//   -> Gbuffer[0](PRE_DEPTH, 뷰공간 위치 재구성), Gbuffer[2](뷰 공간 법선) 읽기
//   -> AO 값(R8_UNORM) 계산 -> Cross-bilateral blur -> mAoRT 에 저장
//   -> LightsPass 가 TextureMaps[GetAOImageIndex()] 로 IBL ambient 에 곱셈
//
// PASS_CUSTOM_DATA 레이아웃:
//   POST_HBAO_PASS (=16)
//     ExtValue[0] = (radius, bias, intensity, numSteps)
//     ExtValue[1] = (numDirs, texelW, texelH, falloff)
//
//   POST_HBAO_BLUR_PASS (=17)
//     ExtValue[0] = (texelW, texelH, float(rawAoIdx), blurRadius)   가로 패스 (etc=0)
//     ExtValue[1] = (texelW, texelH, float(blurRtIdx), blurRadius)  세로 패스 (etc=1)
//
//   LIGHTS_PASS (=18)
//     ExtTex[0] = 최종 AO 텍스처의 TextureMaps 인덱스 (GetAOImageIndex())
//
// GlobalParams 사용:
//   etc (offset 1) : 블러 방향 (0=가로, 1=세로)


struct HbaoRT
{
    shared_ptr<Texture>         tex;
    D3D12_CPU_DESCRIPTOR_HANDLE rtv{};
    uint32                      width  = 0;
    uint32                      height = 0;
};

class HBAOPass : public RenderPass
{
public:
    HBAOPass()  = default;
    ~HBAOPass() = default;

    void Initialize();

    void SetData(
        std::array<PassCustomData,
        static_cast<uint32>(PASS_CUSTOM_INDEX::PASS_CUSTOM_COUNT)>& dataTable,
        RENDER_TARGET_GROUP_TYPE before,
        RENDER_TARGET_GROUP_TYPE after) override;

    void Execute(std::vector<DrawBatch>& batches) override;

    // 최종 AO 텍스처의 TextureMaps
    uint32 GetAOImageIndex() const { return mAoRT.tex->GetImageIndex(); }

    
    void  SetRadius(float r)       { mRadius = r; }        // 뷰 공간 AO 반경(월드 단위)
    float GetRadius()       const  { return mRadius; }     //(1단위 = 1m -> 0.5~2.0)

    void  SetBias(float b)         { mBias = b; }          // 자기-차폐 방지 오프셋
    float GetBias()         const  { return mBias; }

    void  SetIntensity(float i)    { mIntensity = i; }     // AO 최종 강도
    float GetIntensity()    const  { return mIntensity; }

    void  SetFalloff(float f)      { mFalloff = f; }       // 거리 감쇠 계수
    float GetFalloff()      const  { return mFalloff; }

    void  SetNumDirections(int n)  { mNumDirections = n; } // 샘플 방향 수 (4 or 8)
    int   GetNumDirections() const { return mNumDirections; }

    void  SetNumSteps(int n)       { mNumSteps = n; }      // 방향당 스텝 수 (4~8)
    int   GetNumSteps()     const  { return mNumSteps; }

    void  SetBlurRadius(float r)   { mBlurRadius = r; }    // 블러 반경(픽셀)
    float GetBlurRadius()   const  { return mBlurRadius; }

private:
    
    void RunHBAO();
    void RunBlurH(); // 가로 Cross-bilateral blur: mAoRT -> mBlurRT
    void RunBlurV(); // 세로 Cross-bilateral blur: mBlurRT -> mAoRT

    
    HbaoRT CreateHbaoRT(const wstring& name, uint32 w, uint32 h, DXGI_FORMAT fmt);

    void SetViewportAndScissor(uint32 w, uint32 h);
    void RestoreViewportAndScissor();

    void TransitionTo(ID3D12Resource* res,
                      D3D12_RESOURCE_STATES from,
                      D3D12_RESOURCE_STATES to);

    void ClearAndBindRTV(const HbaoRT& rt, float clearVal = 1.0f);

private:

    float mRadius       = 0.5f;
    float mBias         = 0.03f;
    float mIntensity    = 1.5f;
    float mFalloff      = 1.5f;
    int   mNumDirections = 4;
    int   mNumSteps      = 4;
    float mBlurRadius    = 3.0f;

    // 렌더 타깃
    HbaoRT mAoRT;   // Raw HBAO 결과 + V-blur 최종 결과 (둘 다 이 RT 사용)
    HbaoRT mBlurRT; // H-blur 중간 결과

    // mAoRT 의 D3D12 리소스 상태 추적
    // - false(초기/다음 프레임 시작): COMMON
    // - true (Execute 완료~LightsPass 샘플링): PIXEL_SHADER_RESOURCE
    bool mAoRTInPSR = false;


    static constexpr uint32 HBAO_IDX =  static_cast<uint32>(PASS_CUSTOM_INDEX::POST_HBAO_PASS);
    static constexpr uint32 BLUR_IDX =  static_cast<uint32>(PASS_CUSTOM_INDEX::POST_HBAO_BLUR_PASS);
    static constexpr uint32 LIGHTS_IDX =   static_cast<uint32>(PASS_CUSTOM_INDEX::LIGHTS_PASS);
};
