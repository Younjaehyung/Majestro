#include "pch.h"
#include "Scene.h"
#include "Engine.h"
#include "RenderManager.h"
#include "ResourceManager.h"
#include "World.h"
#include "Component.h"
#include "TransformComponent.h"
#include "RenderComponent.h"
#include "CameraComponent.h"
#include "LightComponent.h"
#include "TagComponent.h"
#include "PlayerComponent.h"
#include "AnimationComponent.h"
#include "TerrainComponent.h"
#include "UITransformComponent.h"
#include "UISpriteComponent.h"
#include "BeatComponent.h"
#include "GravityComponent.h"
#include "MovementComponent.h"
#include "VfxComponent.h"
#include "Prefab.h"
//#include "Camera.h"
//
//#include "ConstantBuffer.h"
//#include "Light.h"
//#include "Resources.h"


void Scene::Initialize()
{
	//PlayerPrefab player{mWorld.get()};
	PrefabFactory::RegisterAllPrefabs();
	TerrainPrefab terrain{ mWorld.get() };
	SkyBoxPrefab skybox{ mWorld.get() };
	DirLightPrefab light{ mWorld.get() };
	//EnemyPrefab	enemys {mWorld.get() };


	
	/////////////////////////////////////////////////////////////////////
	{
		Entity vfxEntity = mWorld->CreateEntity();
		TransformComponent vfxTransform{};
		vfxTransform.mLocalPosition = Vec3(0.f, 35.f, 0.f);
		shared_ptr<Vfx> vfx = RESOURCEMANAGER.Get<Vfx>(L"vfx_dissolve_NoteBoar");
		mWorld->AddComponent<TransformComponent>(vfxEntity, vfxTransform);
		VfxComponent& vfxComp = mWorld->AddComponent<VfxComponent>(vfxEntity);
		vfxComp.mVfx = vfx;
	}
	/////////////////////////////////////////////////////////////////////

	{
		Entity osw = mWorld->CreateEntity();	// �ʼ�

		shared_ptr<Mesh> phereMesh = RESOURCEMANAGER.Get<Mesh>(L"Ammor");
		std::vector<shared_ptr<Material>> material2s;

		
		shared_ptr<Material> material2 = RESOURCEMANAGER.Get<Material>(L"oo10");
		material2s.push_back(material2);
		TransformComponent t{};
		t.mLocalPosition = { 0.f, 0.f, 0.f };
		t.mLocalScale = { 1.f, 1.f, 1.f };
		vector<shared_ptr<Animator>> anmators;
		anmators.push_back(RESOURCEMANAGER.Get<Animator>(L"Model|Punch"));
		

		mWorld->AddComponent<TransformComponent>(osw, t);
		mWorld->AddComponent<RenderComponent>(osw, phereMesh, material2s);
		//mWorld->AddComponent<AnimationComponent>(osw, anmators);
		float i, j, k;
		float n = 10;
		for (i = -50; i < 50; i += 10.0f) {
			for (j = -50; j < 50; j += 10.0f) {
				//for (k = -50; k < 50; k += 10.0f) {
					Entity osws = mWorld->CreateEntity();	// �ʼ�
					t.mLocalPosition = { i*n, 0, j*n };


					mWorld->AddComponent<TransformComponent>(osws, t);
					mWorld->AddComponent<RenderComponent>(osws, phereMesh, material2s);
					mWorld->AddComponent<GravityComponent>(osws);
				//}
			}

		}
	}


	/////////////////////////////////////////////////////////////////////////



#pragma region UI
	{
		Entity hpBAR = mWorld->CreateEntity();
		shared_ptr<Material> scorem;
		scorem = RESOURCEMANAGER.Get<Material>(L"HPBAR");

		auto& t = mWorld->AddComponent<UITransformComponent>(hpBAR);
		t.mAnchor = Anchor::TopLeft;
		t.mPosition = Vec2(50.f, 0.f);
		t.mSize = Vec2(512.f, 256.f);


		auto& m = mWorld->AddComponent<UISpriteComponent>(hpBAR,scorem);
	}
	//{
	//	Entity Bass = mWorld->CreateEntity();

	//	shared_ptr<Material> scorem;
	//	scorem = RESOURCEMANAGER.Get<Material>(L"BassPortrait");

	//	auto& t = mWorld->AddComponent<UITransformComponent>(Bass);
	//	t.mAnchor = Anchor::TopLeft;
	//	t.mPosition = Vec2(750.f, 250.f);
	//	t.mSize = Vec2(300.f, 500.f);
	//	


	//	auto& m = mWorld->AddComponent<UISpriteComponent>(Bass, scorem);
	//}
	{
		Entity Bass = mWorld->CreateEntity();

		shared_ptr<Material> scorem;
		scorem = RESOURCEMANAGER.Get<Material>(L"GuitarPortrait");

		auto& t = mWorld->AddComponent<UITransformComponent>(Bass);
		t.mAnchor = Anchor::TopLeft;
		t.mPosition = Vec2(280.f, 110.f);
		t.mSize = Vec2(512.f, 256.f);
		


		auto& m = mWorld->AddComponent<UISpriteComponent>(Bass, scorem);
	}
#pragma endregion

	/////////////////////////////////////////////////////////////////////////


	
	mWorld->Initialize();
}

void Scene::Update(float deltaTime)
{
	mWorld->Update(deltaTime);
}

void Scene::Render()
{
	mWorld->Render();
}


