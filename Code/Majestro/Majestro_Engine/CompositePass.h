#pragma once
#include "PostProcessPass.h"
#include "RenderTarget.h"

class CompositePass
{
public:
	CompositePass() = default;
	~CompositePass() = default;

	void Initialize();
	void Execute(RENDER_TARGET_GROUP_TYPE before, RENDER_TARGET_GROUP_TYPE after);
};

