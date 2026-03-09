#include "pch.h"
#include "PostProcessPass.h"

#include "RenderManager.h"
#include "Engine.h"
#include "ResourceManager.h"

void PostProcess::Initialize()
{
}

void PostProcess::Execute(RENDER_TARGET_GROUP_TYPE before, RENDER_TARGET_GROUP_TYPE after)
{
	if (mEnabled == false) return;
}


void PostProcessPass::Initialize()
{

}

void PostProcessPass::Execute()
{
	mBeforeGroupType = RENDER_TARGET_GROUP_TYPE::POST_PROCESS_A;
	mAfterGroupType = RENDER_TARGET_GROUP_TYPE::POST_PROCESS_B;

	for(shared_ptr<PostProcess>& pass : mPasses)
	{
		pass->Execute(mAfterGroupType, mBeforeGroupType);

		// 다음 패스의 입력이 현재 패스의 출력이 되도록 그룹 타입 교체
		swap(mBeforeGroupType, mAfterGroupType);
	}
}

