#pragma once
#include "RenderPass.h"
#include "RenderTarget.h"

class MotionBlurPass : public RenderPass
{
public:
	MotionBlurPass() = default;
	virtual ~MotionBlurPass() = default;

	virtual void Initialize() override;
	virtual void SetData(std::array<PassCustomData, static_cast<uint32>(PASS_CUSTOM_INDEX::PASS_CUSTOM_COUNT)>& dataTable,
		RENDER_TARGET_GROUP_TYPE before, RENDER_TARGET_GROUP_TYPE after) override;
	virtual void Execute(std::vector<DrawBatch>& deferredDrawBatchs) override;


private:
	Vec2  mPlayerScreenPos{ 0.5f, 0.5f };  // 플레이어 스크린 UV (0~1)
	Vec2  mDashDir{ 1.0f, 0.0f };          // 정규화된 스크린스페이스 이동 방향
	float mDashBlend  = 0.0f;
	float mAspectRatio = 16.0f / 9.0f;
};
