#include "pch.h"
#include "Scene.h"

#include "World.h"
#include "Component.h"
#include "TransformComponent.h"

#include "CameraComponent.h"
#include "LightComponent.h"
#include "TagComponent.h"
#include "PlayerComponent.h"

#include "BeatComponent.h"
#include "GravityComponent.h"
#include "MovementComponent.h"

#include "Prefab.h"
//#include "Camera.h"
//
//#include "ConstantBuffer.h"
//#include "Light.h"
//#include "Resources.h"


void Scene::Initialize()
{
	//PlayerPrefab p{ mWorld.get()};
	PrefabFactory::RegisterAllPrefabs();

	TerrainPrefab terrain{ mWorld.get()};
	//EnemyPrefab	enemys{ mWorld.get() };
	//SkyBoxPrefab skybox{ mWorld };
	//DirLightPrefab light{ mWorld };


	
	/////////////////////////////////////////////////////////////////////
	//{
	//	Entity vfxEntity = mWorld->CreateEntity();
	//	TransformComponent vfxTransform{};
	//	vfxTransform.mLocalPosition = Vec3(0.f, 35.f, 0.f);
	//	shared_ptr<Vfx> vfx = RESOURCEMANAGER.Get<Vfx>(L"vfx_dissolve_NoteBoar");
	//	mWorld->AddComponent<TransformComponent>(vfxEntity, vfxTransform);
	//	VfxComponent& vfxComp = mWorld->AddComponent<VfxComponent>(vfxEntity);
	//	vfxComp.mVfx = vfx;
	//}
	/////////////////////////////////////////////////////////////////////

	//{
	//	Entity osw = mWorld->CreateEntity();	// �ʼ�

	//	shared_ptr<Mesh> phereMesh = RESOURCEMANAGER.Get<Mesh>(L"Ammor");
	//	std::vector<shared_ptr<Material>> material2s;

	//	
	//	shared_ptr<Material> material2 = RESOURCEMANAGER.Get<Material>(L"oo10");
	//	material2s.push_back(material2);
	//	TransformComponent t{};
	//	t.mLocalPosition = { 0.f, 0.f, 0.f };
	//	t.mLocalScale = { 1.f, 1.f, 1.f };
	//	vector<shared_ptr<Animator>> anmators;
	//	anmators.push_back(RESOURCEMANAGER.Get<Animator>(L"Model|Punch"));
	//	

	//	mWorld->AddComponent<TransformComponent>(osw, t);
	//	mWorld->AddComponent<RenderComponent>(osw, phereMesh, material2s);
	//	//mWorld->AddComponent<AnimationComponent>(osw, anmators);
	//	float i, j, k;
	//	float n = 10;
	//	for (i = -50; i < 50; i += 10.0f) {
	//		for (j = -50; j < 50; j += 10.0f) {
	//			//for (k = -50; k < 50; k += 10.0f) {
	//				Entity osws = mWorld->CreateEntity();	// �ʼ�
	//				t.mLocalPosition = { i*n, 0, j*n };


	//				mWorld->AddComponent<TransformComponent>(osws, t);
	//				mWorld->AddComponent<RenderComponent>(osws, phereMesh, material2s);
	//				mWorld->AddComponent<GravityComponent>(osws);
	//			//}
	//		}

	//	}
	//}


	/////////////////////////////////////////////////////////////////////////


	/////////////////////////////////////////////////////////////////////////


	
	mWorld->Initialize();
}

void Scene::Update(float deltaTime)
{
	mWorld->Update(deltaTime);
}



