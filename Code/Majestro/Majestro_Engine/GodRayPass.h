#pragma once
#include "RenderPass.h"
#include "RenderTarget.h"

// Volumetric Light Scattering (VLS) 패스
// 레이마칭 + Cascade Shadow Map 샘플링으로 대기 산란 효과를 계산
class GodRayPass : public RenderPass
{
public:
    GodRayPass() = default;
    virtual ~GodRayPass() = default;

    virtual void Initialize() override;
    virtual void SetData(
        std::array<PassCustomData, static_cast<uint32>(PASS_CUSTOM_INDEX::PASS_CUSTOM_COUNT)>& dataTable,
        RENDER_TARGET_GROUP_TYPE before, RENDER_TARGET_GROUP_TYPE after) override;
    virtual void Execute(std::vector<DrawBatch>& deferredDrawBatchs) override;

    // World Space 태양 방향 설정 (빛이 오는 방향)
    void SetSunDirection(const Vec3& dir) { mSunDir = dir; }


    void SetIntensity(float v)       { mIntensity = v; }
    void SetNumSteps(int v)          { mNumSteps = v; }
    void SetMaxRayLen(float v)       { mMaxRayLen = v; }
    void SetScatterCoeff(float v)    { mScatterCoeff = v; }
    void SetMieAsymmetry(float g)    { mMieG = g; }
    void SetSunColor(const Vec3& c)  { mSunColor = c; }
    void SetAbsorptionCoeff(float v) { mAbsorptionCoeff = v; }

private:
    Vec3  mSunDir          = Vec3(0.f, -1.f, 0.f); // World Space, 태양으로부터 비추는 방향
    float mIntensity       = 0.25f;
    int   mNumSteps        = 32;
    float mMaxRayLen       = 8000.0f;   // 월드 유닛
    float mScatterCoeff    = 0.00008f;  // 산란 계수 (매질 밀도)
    float mMieG            = 0.76f;     // Mie 비대칭 파라미터 (-1~1, 양수=전방산란)
    Vec3  mSunColor        = Vec3(1.0f, 0.97f, 0.82f); // 따뜻한 태양빛
    float mAbsorptionCoeff = 0.00002f;  // 흡수 계수
};
