#pragma once
#include "World.h"
#include "System.h"

class RenderSystem	: public System
{
public:
	RenderSystem(World* world) : System::System(world) {};

	void Update();

	

private:
	void PushLightData();

	void ClearRTV();

	void RenderShadow();

	void RenderDeferred();

	void RenderLights();


	void RenderFinal();	//2pass

	void RenderForward();
};

