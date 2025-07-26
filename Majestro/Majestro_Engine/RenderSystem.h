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
	RenderSystem(World* world);

	void Initialize();
	void Update();

	void MainUpdate() {};
	void PostUpdate() {};

	//void SetMainCamera();
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

	void RenderLightCamera(Entity&, LightComponent*, CameraComponent*);

	void InstancingRender();
	void Render(Entity entity);
	void Render(Entity entity, shared_ptr<InstancingBuffer>& buffer);
private:
	CameraComponent* mCamera;
	uint32 mCullingMask = 0;

	ComponentPool<RenderComponent>* mRenderComponentPool;



	unordered_map<std::wstring&, std::vector<Entity>> shaderBatches;
	 
private:
	std::vector< Entity> mShadowVector;
	std::vector< Entity> mLightVector;
	std::vector< Entity> mDefferdVector;
	std::vector< Entity> mForwardVector;
	std::vector< Entity> mParticleVector;
	

};

