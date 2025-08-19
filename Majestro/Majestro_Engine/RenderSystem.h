#pragma once
#include "World.h"
#include "System.h"
#include "ComponentPool.h"
#include "Buffer.h"
#include "Shader.h"
#include "LightComponent.h"
#include "TransformComponent.h"

struct MaterialParams;
struct PatricleParams;
class Mesh;
class Shader;


struct RenderParams 
{
	uint32 ObjectIndex{};
	uint32 MaterialInfoIndex;

	uint32 LightIndex;    //light가 아니면 쓰지 말것.
	uint32 ParticleIndex; //Particle가 아니면 쓰지 말것.
};


struct PassParams
{
	Matrix MatView;
	Matrix MatProjection;
	Matrix MatViewInv; // view의 역행렬
	Matrix MatProjectionInv; // Projection의 역행렬	(사용은 선택)

	Vec2 ScreenSize{};
	Vec2 Padding{};

	uint32 LightsCount{};
	uint32 SkyBoxIndex{};
};

struct DrawItem 
{
	shared_ptr<Shader> PSOShader{};
	shared_ptr<Mesh> PMesh{};

	uint32 PSOID{};
	uint32 MeshID{};
	uint32 SubMesh{};
	uint32 SubMeshIndex{};

	RenderParams InstanceGPU;
	DrawItem() = default;
	DrawItem(shared_ptr<Shader> shader,
		shared_ptr<Mesh> mesh,
		uint32 psoID,
		uint32 meshID,
		uint32 subMesh,
		uint32  subMeshIndex,
		RenderParams instanceGPU) {
		PSOShader = shader;
		PMesh = mesh;
		PSOID = psoID;
		MeshID = meshID;
		SubMesh = subMesh;
		SubMesh = subMeshIndex;
		InstanceGPU = instanceGPU;

	}
};

struct DrawBatch
{
	shared_ptr<Shader> PSOShader{};
	shared_ptr<Mesh> Mesh{};

	uint32 PSOID{};
	uint32 MeshID{};
	uint32 SubMesh{};
	uint32 SubMeshIndex{};

	uint32 BaseInstance{};
	uint32 InstanceCount{};
	RenderParams InstanceGPU{};

	DrawBatch() = default;
	DrawBatch(DrawItem* drawItem){
		PSOShader = drawItem->PSOShader;
		Mesh = drawItem->PMesh;
		PSOID = drawItem->PSOID;
		MeshID = drawItem->MeshID;
		SubMesh = drawItem->SubMesh;
		SubMeshIndex = drawItem->SubMeshIndex;
		InstanceGPU = drawItem->InstanceGPU;

	}

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

	void PushData();

	void DefferdRendering();
	void ForwardRendering();
	void ParticleRendering();



private: // Culling
	bool IsCustomCulled(uint8 layer) { return (mCullingMask & (1 << layer)) != 0; }
	bool IsFrustumCulled();


private: // Push&Clear Data
	void PushMaterialData();

	void SetTable();
	void PushPassData();
	void PushInstanceData();
	void PushGBufferData();
	void PushObjectData();
	void PushLightData();
	void ClearBuffer();

private: // Render

	void RenderShadowCamera(Entity&, LightComponent*, class CameraComponent*,class RenderComponent*);
	void InstancingRender(DrawBatch&);


	// deferred
	void RenderShadow();

	void RenderGBuffer();

	void RenderLights();

	void RenderFinal();	//2pass //clear

	// forward
	void RenderForward();

	// particle
	void RenderingParticle();

private:
	uint32 mFrameCount = 0;

	Entity				mCameraID;
	class CameraComponent* mCamera{};
	uint32				mCullingMask = 0;


	shared_ptr<RootSignature>		mRootSignature;
	ComponentPool<RenderComponent>* mRenderComponentPool=nullptr;
	
private:	// 배치 버퍼

	// Pass별 PSO에 따른 분류
	std::vector<DrawItem> mDeferredDrawItems;
	
	std::vector<DrawBatch> mDeferredDrawBatchs;

	

private:
	
	std::vector<Entity>				mDummyVector;
	// RenderManager의 structuerdBuffer로 복사할 데이터들

	std::vector<RenderParams>		mInstanceVector;
	std::vector<LightParams>		mLightVector;
	std::vector<ObjectParams>		mObjectVector;
	std::vector<MaterialParams>		mMaterialVector;
	std::vector<PatricleParams>		mPatricleVector;

private:
	// 변수 재사용을 막기 위해 둔 Dummy Parms
	PassParams		passParams{};
	ObjectParams	objectParams{};
	LightParams		lightParams{};
	RenderParams	renderParams{};

	DrawBatch mBatch{};
	uint32  mCurrPSOID{};
};

