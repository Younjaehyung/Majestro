#pragma once
#include "World.h"
#include "System.h"
#include "ComponentPool.h"
#include "Buffer.h"
#include "Shader.h"
#include "TransformComponent.h"
#include "LightComponent.h"

class CameraComponent;
class RenderComponent;


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
	void PushGBuffer();
	void PushPassData();
	void PushObjectData(TransformComponent* transformComponent);
	void PushMaterialData(RenderComponent* renderComponent);
	void ClearBuffer();

private: // Render
	void RenderShadowCamera(Entity&, LightComponent*, CameraComponent*);

//	void Render(Entity entity);
//	void Render(Entity entity, shared_ptr<InstancingBuffer>& buffer);
	void InstancingRender(vector<Entity>& gameObjects);
	
private:
	uint8 mFrameCount = 0;

	Entity				mCameraID;
	CameraComponent*	mCamera;
	uint32				mCullingMask = 0;


	shared_ptr<RootSignature>		mRootSignature;
	ComponentPool<RenderComponent>* mRenderComponentPool;
	
private:	// 배치 버퍼

	// 동일 머티리얼을 사용하는 오브젝트별로 분류	(머테리얼 배치)
	unordered_map<uint64, vector<Entity>> mMaterialObjectBatch; 

	// 쉐이더 타입별로 | 쉐이더 종류별로 분류	(쉐이더 배치)
	array<unordered_map<std::wstring, std::vector<Entity>>, static_cast<uint8>(SHADER_TYPE::END)> shaderBatches;
private:

	// RenderManager의 structuerdBuffer로 복사할 데이터들
	std::vector< LightParams>		mLightVector;
	std::vector< ObjectParams>	mTransformVector;
	std::vector<struct MaterialParams>	mMaterialVector;
	std::vector<struct PatricleParams>	mPatricleVector;

	// lightRenderPASS 때 사용될 vector
	std::vector<struct MaterialParams>	mLightCulling;

private:
	// 변수 재사용을 막기 위해 둔 Dummy Parms
	ObjectParams objectParams = {};
	LightParams	lightParams = {};
};

