#pragma once
#include "RenderPass.h"
#include "RenderSystem.h"

class DepthPrePass : public RenderPass{
public:
    void Initialize();
	void SetData(std::array<PassCustomData, static_cast<uint32>(PASS_CUSTOM_INDEX::PASS_CUSTOM_COUNT)>& dataTable, RENDER_TARGET_GROUP_TYPE before, RENDER_TARGET_GROUP_TYPE after) override;
    void Execute(std::vector<DrawBatch>& drawBatchs);

    void InstancingRender(DrawBatch& drawBatch);
private:
    uint32 mCurrPSOID{};
    struct dummy { uint32 BaseInstance, InstanceCount, Cascade; } dum;

	shared_ptr<Shader> depthShader;      // 일반 불투명 depth prepass PSO
	shared_ptr<Shader> depthShaderAlpha; // 알파 컷아웃 식생 depth prepass PSO
};

