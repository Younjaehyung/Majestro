#pragma once
#include "World.h"
#include "System.h"
#include "Instancing.h"
#include "ComponentPool.h"

class CameraComponent;
class RenderComponent;

class RenderSystem	: public System
{
public:
	RenderSystem(World* world) : System::System(world) {};

	void Initialize();
	void Update();

	

private:
	

	void PushLightData();

	void ClearRTV();

	void RenderShadow();

	void RenderDeferred();

	void RenderLights();


	void RenderFinal();	//2pass

	void RenderForward();
private:
	bool IsCustomCulled(uint8 layer) { return (mCullingMask & (1 << layer)) != 0; }
	bool IsFrustumCulled();


private:
	CameraComponent* mCamera;
	uint32 mCullingMask = 0;

	ComponentPool<RenderComponent>* mRenderComponentPool;


	InstancingManager* mInstancingManager;
};

