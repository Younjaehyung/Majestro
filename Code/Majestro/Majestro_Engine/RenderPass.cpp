#include "pch.h"
#include "RenderPass.h"
#include "RenderSystem.h"

#include "Engine.h"
#include "ResourceManager.h"
#include "RenderManager.h"

#include "RenderSystem.h"
#include "ToneMapPass.h"
#include "FinalCompositePass.h"
#include "ChromaticAberrationPass.h"
#include "MotionBlurPass.h"



void RenderPass::Initialize()
{
}

void RenderPass::SetData(std::array<PassCustomData, static_cast<uint32>(PASS_CUSTOM_INDEX::PASS_CUSTOM_COUNT)>& dataTable,
    RENDER_TARGET_GROUP_TYPE before, RENDER_TARGET_GROUP_TYPE after)
{
    mBefore = before;
    mAfter = after;
}

void RenderPass::Execute(std::vector<DrawBatch>& deferredDrawBatchs)
{
	if (mEnabled == false) return;
}

void RenderPass::InstancingRender(DrawBatch& drawBatch)
{
    drawBatch.Mesh->Render(drawBatch.InstanceCount, drawBatch.SubMeshIndex,
        0, 0 /*drawBatch.SubMeshIndex+ drawBatch.ParamsINX*/);
}




void PostProcessPass::Initialize()
{
	mToneMapPass = make_shared<ToneMapPass>();
	
    mFinalCompositePass = make_shared<FinalCompositePass>();

  

   // AddLDRPass(std::make_shared<ChromaticAberrationPass>());
   
}

void PostProcessPass::SetData(std::array<PassCustomData, static_cast<uint32>(PASS_CUSTOM_INDEX::PASS_CUSTOM_COUNT)>& passTable)
{
    RENDER_TARGET_GROUP_TYPE hdrBefore = RENDER_TARGET_GROUP_TYPE::HDR;
    RENDER_TARGET_GROUP_TYPE hdrAfter = RENDER_TARGET_GROUP_TYPE::POST_HDR_A;

    bool firstHDRPass = true;

    for (shared_ptr<RenderPass>& pass : mHDRPasses)
    {
		if (pass->IsEnabled() == false) continue;
      
        pass->SetData(passTable, hdrBefore, hdrAfter);

        if (firstHDRPass)
        {
            // 첫 패스 이후부터는 POST_HDR_A/B ping-pong만 사용
            hdrBefore = hdrAfter;
            hdrAfter = RENDER_TARGET_GROUP_TYPE::POST_HDR_B;
            firstHDRPass = false;
        }
        else
        {
            swap(hdrBefore, hdrAfter);
        }
        // POST_HDR Group 초기화


    }

    // HDR 패스가 하나도 없으면 scene color를 그대로 tone map
    if (mHDRPasses.empty())
    {

        hdrBefore = RENDER_TARGET_GROUP_TYPE::HDR;
    }

    RENDER_TARGET_GROUP_TYPE ldrBefore = RENDER_TARGET_GROUP_TYPE::POST_LDR_A; //9
    RENDER_TARGET_GROUP_TYPE ldrAfter = RENDER_TARGET_GROUP_TYPE::POST_LDR_B;  //10

    // 수정: ToneMap은 HDR 입력 -> LDR 출력이어야 함
    mToneMapPass->SetData(passTable, hdrBefore, ldrBefore);

    for (shared_ptr<RenderPass>& pass : mLDRPasses)
    {
        if (pass->IsEnabled() == false) continue;
        pass->SetData(passTable, ldrBefore, ldrAfter);
        swap(ldrBefore, ldrAfter);
        // POST_LDR Group 초기화

    }

    // 수정: UI / FinalComposite가 참조할 최종 결과 저장
    mFinalCompositePass->SetData(passTable, ldrBefore, ldrBefore);
}

void PostProcessPass::SetColorGrading(const ColorGradingParams& params)
{
	if (mToneMapPass) mToneMapPass->SetColorGrading(params);
}

const ColorGradingParams& PostProcessPass::GetColorGrading() const
{
	static const ColorGradingParams sDefault{};
	if (!mToneMapPass) return sDefault;
	return mToneMapPass->GetColorGrading();
}

void PostProcessPass::Execute(std::vector<DrawBatch>& deferredDrawBatchs)
{
    for (shared_ptr<RenderPass>& pass : mHDRPasses)
    {
        if (pass->IsEnabled() == false) continue;
        pass->Execute(deferredDrawBatchs);
    }

    // 수정: ToneMap은 HDR 입력 -> LDR 출력이어야 함
    mToneMapPass->Execute(deferredDrawBatchs);

    for (shared_ptr<RenderPass>& pass : mLDRPasses)
    {
        if (pass->IsEnabled() == false) continue;
        pass->Execute(deferredDrawBatchs);
    }

    // 수정: UI / FinalComposite가 참조할 최종 결과 저장
    mFinalCompositePass->Execute(deferredDrawBatchs);
}
