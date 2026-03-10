#include "pch.h"
#include "PostProcessPass.h"

#include "RenderManager.h"
#include "Engine.h"
#include "ResourceManager.h"

#include "ToneMapPass.h"
#include "FinalCompositePass.h"
#include "ChromaticAberrationPass.h"



void PostProcess::Initialize()
{
}

void PostProcess::Execute(RENDER_TARGET_GROUP_TYPE before, RENDER_TARGET_GROUP_TYPE after)
{
	if (mEnabled == false) return;
}


void PostProcessPass::Initialize()
{
	mToneMapPass = make_shared<ToneMapPass>();
	mFinalCompositePass = make_shared<FinalCompositePass>();

    // AddLDRPass(std::make_shared<ChromaticAberrationPass>());
}

void PostProcessPass::Execute()
{
    RENDER_TARGET_GROUP_TYPE hdrBefore = RENDER_TARGET_GROUP_TYPE::HDR;
    RENDER_TARGET_GROUP_TYPE hdrAfter = RENDER_TARGET_GROUP_TYPE::POST_HDR_A;

    bool firstHDRPass = true;

    for (shared_ptr<PostProcess>& pass : mHDRPasses)
    {
        pass->Execute(hdrBefore, hdrAfter);

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
    }

    // HDR 패스가 하나도 없으면 scene color를 그대로 tone map
    if (mHDRPasses.empty())
    {
        hdrBefore = RENDER_TARGET_GROUP_TYPE::HDR;
    }

    RENDER_TARGET_GROUP_TYPE ldrBefore = RENDER_TARGET_GROUP_TYPE::POST_LDR_A;
    RENDER_TARGET_GROUP_TYPE ldrAfter = RENDER_TARGET_GROUP_TYPE::POST_LDR_B;

    // 수정: ToneMap은 HDR 입력 -> LDR 출력이어야 함
    mToneMapPass->Execute(hdrBefore, ldrBefore);

    for (shared_ptr<PostProcess>& pass : mLDRPasses)
    {
        pass->Execute(ldrBefore, ldrAfter);
        swap(ldrBefore, ldrAfter);
    }

    // 수정: UI / FinalComposite가 참조할 최종 결과 저장
    mFinalCompositePass->Execute(ldrBefore);
}
