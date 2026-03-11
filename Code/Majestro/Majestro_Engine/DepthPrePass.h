#pragma once
#include "RenderSystem.h"

class DepthPrePass {
public:
    void Initialize();
    void Execute(std::vector<DrawBatch>& drawBatchs);

    void InstancingRender(DrawBatch& drawBatch);
private:
    uint32 mCurrPSOID{};
    struct dummy { uint32 BaseInstance, InstanceCount, Cascade; } dum;

	shared_ptr<Shader> depthShader; // depth prepass용 PSO
};

