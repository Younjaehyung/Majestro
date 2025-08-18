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

struct RenderParams {
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
	void PushGBufferData();
	void PushObjectData();
	void PushLightData();
	void ClearBuffer();

private: // Render

	void RenderShadowCamera(Entity&, LightComponent*, class CameraComponent*,class RenderComponent*);
	void InstancingRender(vector<Entity>& gameObjects);


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

	// 동일 머티리얼을 사용하는 오브젝트별로 분류	(머테리얼 배치)
	unordered_map<uint64, vector<Entity>> mMaterialObjectBatch; 

	// 쉐이더 타입별로 | 쉐이더 종류별로 분류	(쉐이더 배치)
	array<unordered_map<std::wstring, std::vector<Entity>>, static_cast<uint8>(SHADER_TYPE::END)> shaderBatches;
private:
	std::vector<Entity>				mDummyVector;
	// RenderManager의 structuerdBuffer로 복사할 데이터들

	std::vector<LightParams>		mLightVector;
	std::vector<ObjectParams>		mObjectVector;
	std::vector<MaterialParams>		mMaterialVector;
	std::vector<PatricleParams>		mPatricleVector;

private:
	// 변수 재사용을 막기 위해 둔 Dummy Parms
	PassParams passParams{};
	ObjectParams objectParams{};
	LightParams	lightParams{};
	RenderParams	renderParams{};
};

