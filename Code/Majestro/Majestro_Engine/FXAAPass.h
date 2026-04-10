#pragma once
#include "RenderPass.h"
#include "RenderTarget.h"

class FXAAPass : public RenderPass
{
public:
    FXAAPass()  = default;
    ~FXAAPass() = default;

    void Initialize() override;

    void SetData(
        std::array<PassCustomData, static_cast<uint32>(PASS_CUSTOM_INDEX::PASS_CUSTOM_COUNT)>& dataTable,
        RENDER_TARGET_GROUP_TYPE before,
        RENDER_TARGET_GROUP_TYPE after) override;

    void Execute(std::vector<DrawBatch>& deferredDrawBatchs) override;


    // edgeThreshold    : 0.063(품질 최고) ~ 0.333(성능 최고)
    // edgeThresholdMin : 0.031(품질 최고) ~ 0.0833(성능 최고)
    // subpixQuality    : 0.0(꺼짐) ~ 1.0(최대 블렌드)
    void SetParams(float edgeThreshold, float edgeThresholdMin, float subpixQuality);

    float GetEdgeThreshold()    const { return mEdgeThreshold;    }
    float GetEdgeThresholdMin() const { return mEdgeThresholdMin; }
    float GetSubpixQuality()    const { return mSubpixQuality;    }

private:
    static constexpr uint32 FXAA_IDX =
        static_cast<uint32>(PASS_CUSTOM_INDEX::POST_FXAA_PASS);

    float mEdgeThreshold    = 0.125f;
    float mEdgeThresholdMin = 0.0625f;
    float mSubpixQuality    = 0.75f;
};
