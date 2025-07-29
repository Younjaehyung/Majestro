#pragma once
#include "World.h"
#include "System.h"
#include "ComponentPool.h"
#include "Buffer.h"
#include "Shader.h"

class CameraComponent;
class RenderComponent;
class LightComponent;
class TransformComponent;


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
	

	void PushLightData(); //clear

	void ClearRTV();	//clear

	void RenderShadow();	//clear

	void RenderDeferred();	//

	void RenderLights();

	void RenderFinal();	//2pass //clear

	void RenderForward();


private:
	bool IsCustomCulled(uint8 layer) { return (mCullingMask & (1 << layer)) != 0; }
	bool IsFrustumCulled();

	void RenderShadowCamera(Entity&, LightComponent*, CameraComponent*);

	void InstancingRender(vector<Entity>& gameObjects);
	void AddParam(uint64 instanceId, InstancingParams& data);
	void PushTransformData(TransformComponent* transformComponent);
	void ClearBuffer();


	void Render(Entity entity);
	void Render(Entity entity, shared_ptr<InstancingBuffer>& buffer);
private:
	CameraComponent* mCamera;
	uint32 mCullingMask = 0;
	shared_ptr<RootSignature>mRootSignature;

	ComponentPool<RenderComponent>* mRenderComponentPool;
	unordered_map<uint64/*instanceId*/, shared_ptr<InstancingBuffer>> _buffers;


	//unordered_map<std::wstring, std::vector<Entity>> shaderBatches;
	unordered_map<uint64, vector<Entity>> cache;
	array<unordered_map<std::wstring, std::vector<Entity>>, static_cast<uint8>(SHADER_TYPE::END)> shaderBatches;
private:
	
};

