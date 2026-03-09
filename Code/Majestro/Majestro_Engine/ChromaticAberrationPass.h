#pragma once
#include "PostProcessPass.h"
#include "RenderTarget.h"

class ChromaticAberrationPass : public PostProcess
{
public:
	ChromaticAberrationPass() = default;
	virtual ~ChromaticAberrationPass() = default;

	virtual void Initialize();

	virtual void Execute(RENDER_TARGET_GROUP_TYPE, RENDER_TARGET_GROUP_TYPE);
};

