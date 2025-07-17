#pragma once
#include "System.h"

class RenderSystem	: public System
{
public:
	void Update(float deltaTime);

	

private:
	void PushLightData();

	void ClearRTV();

	void RenderShadow();

	void RenderDeferred();

	void RenderLights();


	void RenderFinal();	//2pass

	void RenderForward();
};

