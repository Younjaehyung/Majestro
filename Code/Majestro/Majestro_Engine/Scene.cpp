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
#include "VfxComponent.h"
#include "Prefab.h"
//#include "Camera.h"
//
//#include "ConstantBuffer.h"
//#include "Light.h"
//#include "Resources.h"


void Scene::Initialize()
{

	TransformComponent t{};
	Entity testCamera = mWorld->CreateEntity();
	mWorld->AddComponent<MainCameraComponent>(testCamera);
	mWorld->AddComponent<CameraComponent>(testCamera);
	mWorld->AddComponent<TransformComponent>(testCamera,t);


	/////////////////////////////////////////////////////////////////////
	//Entity testEntity = mWorld->CreateEntity();	// �ʼ�

	//shared_ptr<Mesh> sphereMesh = RESOURCEMANAGER.LoadSphereMesh();
	//shared_ptr<Material> material = RESOURCEMANAGER.Get<Material>(L"GameObject");
	//std::vector<shared_ptr<Material>> materials;
	//materials.push_back(material);

	//TransformComponent t{};
	//t.mLocalScale = { 100.f, 100.f, 100.f };
	//t.mLocalPosition = {0.f, 0.f, 500.f};




	//mWorld->AddComponent<TransformComponent>(testEntity, t);
	//mWorld->AddComponent<RenderComponent>(testEntity, sphereMesh, materials);
	//////////////////////////////////////////////////////////////
	//Entity testEntity = mWorld->CreateEntity();	// �ʼ�

	//shared_ptr<Mesh> sphereMesh = RESOURCEMANAGER.Get<Mesh>(L"Dragon_Mesh");
	//std::vector<shared_ptr<Material>> materials;
	//
	//shared_ptr<Material> material = RESOURCEMANAGER.Get<Material>(L"Dragon0");
	//materials.push_back(material);
	//material = RESOURCEMANAGER.Get<Material>(L"Dragon1");
	//materials.push_back(material);
	//material = RESOURCEMANAGER.Get<Material>(L"Dragon2");
	//materials.push_back(material);
	//material = RESOURCEMANAGER.Get<Material>(L"Dragon3");
	//materials.push_back(material);
	//material = RESOURCEMANAGER.Get<Material>(L"Dragon4");
	//materials.push_back(material);
	//



	//mWorld->AddComponent<TransformComponent>(testEntity, t);
	//mWorld->AddComponent<RenderComponent>(testEntity, sphereMesh, materials);
	/////////////////////////////////////////////////////////////////////

#pragma region Skybox
	{
		Entity skyBox = mWorld->CreateEntity();
		TransformComponent bt{};


		shared_ptr<Mesh> skyBoxMesh = RESOURCEMANAGER.Get<Mesh>(L"Sphere");

		// 빌보드 머티리얼
		shared_ptr<Material> skyBoxMat = RESOURCEMANAGER.Get<Material>(L"Skybox");
		std::vector<shared_ptr<Material>> materials;
		materials.push_back(skyBoxMat);

		mWorld->AddComponent<TransformComponent>(skyBox, bt);
		RenderComponent& render = mWorld->AddComponent<RenderComponent>(skyBox, skyBoxMesh, materials);
		render.mCheckFrustum = false;
	}
#pragma endregion

	{
		Entity osw = mWorld->CreateEntity();	// �ʼ�

		shared_ptr<Mesh> phereMesh = RESOURCEMANAGER.Get<Mesh>(L"Rudwig_mBody");
		//shared_ptr<Mesh> phereMesh = RESOURCEMANAGER.Get<Mesh>(L"Guitar_mBody");
		std::vector<shared_ptr<Material>> material2s;

		shared_ptr<Material> material2 = RESOURCEMANAGER.Get<Material>(L"Rudwig_aIdle_0010");
		//shared_ptr<Material> material2 = RESOURCEMANAGER.Get<Material>(L"Capoeira0");

		material2s.push_back(material2);
		t.mLocalPosition = { 0.f, 0.f, 10.f };
		t.mLocalScale = { 10.f, 10.f, 10.f };

		vector<shared_ptr<Animator>> anmators0;
		anmators0.push_back(RESOURCEMANAGER.Get<Animator>(L"Armature|Rudwig_aIdle_001"));
		anmators0.push_back(RESOURCEMANAGER.Get<Animator>(L"Armature|Rudwig_aWalk_001"));
		anmators0.push_back(RESOURCEMANAGER.Get<Animator>(L"Armature|Rudwig_aRun_001"));
		anmators0.push_back(RESOURCEMANAGER.Get<Animator>(L"Armature|Rudwig_aJump_001"));
		anmators0.push_back(RESOURCEMANAGER.Get<Animator>(L"Armature|Rudwig_aRun_001"));//dash

		//anmators.push_back(RESOURCEMANAGER.Get<Animator>(L"mixamo.com"));

		mWorld->AddComponent<ControllerComponent>(osw,t, THREE_FPS);
		mWorld->AddComponent<MainPlayerComponent>(osw, "../Resources/Json/TestJson.json", anmators0);
		mWorld->AddComponent<TransformComponent>(osw, t);
		mWorld->AddComponent<RenderComponent>(osw, phereMesh, material2s);
		mWorld->AddComponent<AnimationComponent>(osw, anmators0);
		mWorld->AddComponent<BeatComponent>(osw);


		/*
		float i, j;

		for (i = -51; i < 0; i += 51.0f) {
			for (j = -51; j < 0; j += 51.0f) {
				Entity osws = mWorld->CreateEntity();	// �ʼ�
				t.mLocalPosition = { i, j, 100.f };


				mWorld->AddComponent<TransformComponent>(osws, t);
				mWorld->AddComponent<RenderComponent>(osws, phereMesh, material2s);
				//mWorld->AddComponent<AnimationComponent>(osws, anmators);
			}

		}*/
	}
	{
		Entity osw = mWorld->CreateEntity();	// �ʼ�

		shared_ptr<Mesh> phereMesh = RESOURCEMANAGER.Get<Mesh>(L"Ammor");
		std::vector<shared_ptr<Material>> material2s;

		
		shared_ptr<Material> material2 = RESOURCEMANAGER.Get<Material>(L"oo10");
		material2s.push_back(material2);
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
					//mWorld->AddComponent<AnimationComponent>(osws, anmators);
				//}
			}

		}
	}


	/////////////////////////////////////////////////////////////////////////

#pragma region Terrain

	Entity terrain = mWorld->CreateEntity();
	TransformComponent bt{};
	bt.mLocalScale = (Vec3(30.f, 250.f, 30.f));
	bt.mLocalPosition = Vec3(-150.f, -70.f, -150.f);

	shared_ptr<Mesh> skyBoxMesh = RESOURCEMANAGER.LoadTerrainMesh(64, 64);

	// 빌보드 머티리얼(
	shared_ptr<Material> heightMap = RESOURCEMANAGER.Get<Material>(L"Terrain");
	std::vector<shared_ptr<Material>> materials;
	materials.push_back(heightMap);

	mWorld->AddComponent<TransformComponent>(terrain, bt);
	TerrainComponent& terrainc = mWorld->AddComponent<TerrainComponent>(terrain, 64, 64, heightMap);
	terrainc.mTerrainWorldPosition = bt.mLocalPosition;
	terrainc.mTerrainWorldScale = bt.mLocalScale;

	RenderComponent& render = mWorld->AddComponent<RenderComponent>(terrain, skyBoxMesh, materials);
	render.mCheckFrustum = false;



#pragma endregion

#pragma region VFX
	{
		Entity vfxEntity = mWorld->CreateEntity();
		TransformComponent vfxTransform{};
		vfxTransform.mLocalPosition = Vec3(-20.f, 15.f, 0.f);
		shared_ptr<Vfx> vfx = RESOURCEMANAGER.Get<Vfx>(L"fire");
		mWorld->AddComponent<TransformComponent>(vfxEntity, vfxTransform);
		VfxComponent& vfxComp = mWorld->AddComponent<VfxComponent>(vfxEntity);
		vfxComp.mVfx = vfx;
	}
#pragma endregion

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
	LightComponent l{};
	l.mLightInfo.Position = {Vec3(0, 1000, 500)};
	l.mLightInfo.Color.Ambient = { Vec3(0.1f, 0.1f, 0.1f) };
	l.mLightInfo.Color.Diffuse = { Vec3(1.f, 1.f, 1.f) };
	l.mLightInfo.Color.Specular = { Vec3(0.1f, 0.1f, 0.1f) };
	l.SetLightDirection(Vec3(0, -1, 1.f));

	LightFactory::CreateLight(mWorld,LIGHT_TYPE::DIRECTIONAL_LIGHT,l);

	//Particle
	//particleComponent->_particleBuffer->PushGraphicsData(SRV_REGISTER::t9);
	//particleComponent->_material->SetFloat(0, particleComponent->_startScale);
	//particleComponent->_material->SetFloat(1, particleComponent->_endScale);
	//particleComponent->_material->PushGraphicsData();

	//TransformComponent t;


	//testlight->GetLight()->SetLightDirection(Vec3(0, -1, 1.f));
	//testlight->GetLight()->SetLightType(LIGHT_TYPE::DIRECTIONAL_LIGHT);
	//testlight->GetLight()->SetDiffuse(Vec3(1.f, 1.f, 1.f));
	//testlight->GetLight()->SetAmbient(Vec3(0.1f, 0.1f, 0.1f));
	//testlight->GetLight()->SetSpecular(Vec3(0.1f, 0.1f, 0.1f));


	//mWorld->Awake();
}

void Scene::Update(float deltaTime)
{
	mWorld->Update(deltaTime);
}

void Scene::Render()
{
	mWorld->Render();
}


