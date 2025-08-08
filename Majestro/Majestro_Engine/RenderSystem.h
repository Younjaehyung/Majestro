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

struct PassParams
{
	Matrix MatView;
	Matrix MatProjection;
	Matrix MatViewInv; // view의 역행렬
	Matrix MatProjectionInv; // Projection의 역행렬	(사용은 선택)

	Vec2 ScreenSize;
	Vec2 Padding;

	int LightsCount;
	int SkyBoxIndex;
};

class RenderSystem	: public System
{
public:
	RenderSystem(World* world);

	void Initialize();
	void Update();

	void MainUpdate() {};
	void PostUpdate() {};

	
private: // RenderPass
	void ClearRTV();	//clear

	void PushLightData(); //clear
	void PushObjectData(); 


	void RenderShadow();	

	void RenderDeferred();	//

	void RenderLights();

	void RenderFinal();	//2pass //clear

	void RenderForward();


private: // Culling
	bool IsCustomCulled(uint8 layer) { return (mCullingMask & (1 << layer)) != 0; }
	bool IsFrustumCulled();


private: // Push&Clear Data
	void PushTransformData(TransformComponent* transformComponent);
	void PushMaterialData(RenderComponent* renderComponent);
	void ClearBuffer();

private: // Render
	void RenderShadowCamera(Entity&, LightComponent*, CameraComponent*);

//	void Render(Entity entity);
//	void Render(Entity entity, shared_ptr<InstancingBuffer>& buffer);
	void InstancingRender(vector<Entity>& gameObjects);
	
private:
	uint8 mFrameCount = 0;

	CameraComponent* mCamera;
	uint32 mCullingMask = 0;

	//static_cast<float>(window.Width), static_cast<float>(window.Height)

	shared_ptr<RootSignature>mRootSignature;
	ComponentPool<RenderComponent>* mRenderComponentPool;
	
private:
	// 배치 버퍼
	unordered_map<uint64, vector<Entity>> mMaterialObjectBatch;
	array<unordered_map<std::wstring, std::vector<Entity>>, static_cast<uint8>(SHADER_TYPE::END)> shaderBatches;
private:

	// RenderManager의 structuerdBuffer로 복사할 데이터들
	std::vector<struct LightParams>		mLightVector;
	std::vector<struct TransformParams>	mTransformVector;
	std::vector<struct MaterialParams>	mMaterialVector;

	// lightRenderPASS 때 사용될 vector
	std::vector<struct MaterialParams>	mLightCulling;
};

