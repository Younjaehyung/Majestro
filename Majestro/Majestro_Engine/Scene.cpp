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
#include "AnimationComponent.h"

#include "Prefab.h"
//#include "Camera.h"
//
//#include "ConstantBuffer.h"
//#include "Light.h"
//#include "Resources.h"


void Scene::Initialize()
{
	Entity testCamera = mWorld->CreateEntity();
	mWorld->AddComponent<MainCameraComponent>(testCamera);
	mWorld->AddComponent<CameraComponent>(testCamera);
	mWorld->AddComponent<TransformComponent>(testCamera);


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

	TransformComponent t{};

	t.mLocalPosition = {0.f, 0.f, 50.f};




	//mWorld->AddComponent<TransformComponent>(testEntity, t);
	//mWorld->AddComponent<RenderComponent>(testEntity, sphereMesh, materials);
	/////////////////////////////////////////////////////////////////////
	Entity osw = mWorld->CreateEntity();	// �ʼ�

	shared_ptr<Mesh> phereMesh = RESOURCEMANAGER.Get<Mesh>(L"Ammor");
	std::vector<shared_ptr<Material>> material2s;
	
	shared_ptr<Material> material2 = RESOURCEMANAGER.Get<Material>(L"oo10");
	material2s.push_back(material2);
	t.mLocalPosition = { 0.f, 0.f, 50.f };

	vector<shared_ptr<Animator>> anmators;
	anmators.push_back(RESOURCEMANAGER.Get<Animator>(L"Model|Punch"));

	mWorld->AddComponent<TransformComponent>(osw, t);
	mWorld->AddComponent<RenderComponent>(osw, phereMesh, material2s);
	mWorld->AddComponent<AnimationComponent>(osw, anmators);
	float i, j;
	for (i = -10; i < 10; i+=1.0f) {
		for (j = -10; j < 10; j += 1.0f) {
			Entity osws = mWorld->CreateEntity();	// �ʼ�
			t.mLocalPosition = { i, j, 50.f };


			mWorld->AddComponent<TransformComponent>(osws, t);
			mWorld->AddComponent<RenderComponent>(osws, phereMesh, material2s);
			mWorld->AddComponent<AnimationComponent>(osws, anmators);
		}
	}



	/////////////////////////////////////////////////////////////////////////
	//Entity testEntity5 = mWorld->CreateEntity();	// �ʼ�


	//TransformComponent ts{};
	//ts.mLocalScale = { 100.f, 100.f, 100.f };
	//ts.mLocalPosition = { 50.f, -10.f, 500.f };




	//mWorld->AddComponent<TransformComponent>(testEntity5, ts);
	//mWorld->AddComponent<RenderComponent>(testEntity5, sphereMesh, materials);

	//// 한 번에 32x32 = 1024개 스폰 (간격, 스케일은 취향대로)
	//const int NX = 520;

	//const float SPACING = 12.f;


	//	for (int x = 0; x < NX; ++x)
	//	{
	//		Entity e = mWorld->CreateEntity();

	//		TransformComponent t{};
	//		t.mLocalScale = { 100.f, 100.f, 100.f };
	//		t.mLocalPosition = { -350.f + x * SPACING, -10.f, 500.f };

	//		mWorld->AddComponent<TransformComponent>(e, t);
	//		mWorld->AddComponent<RenderComponent>(e, sphereMesh, materials);
	//	}
	//

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


