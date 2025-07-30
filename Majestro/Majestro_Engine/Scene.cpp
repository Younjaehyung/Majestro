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

	Entity testEntity = mWorld->CreateEntity();
	shared_ptr<Mesh> sphereMesh = RESOURCEMANAGER.LoadSphereMesh();
	shared_ptr<Material> material = RESOURCEMANAGER.Get<Material>(L"GameObject");
	std::vector<shared_ptr<Material>> materials;
	materials.push_back(material);
	TransformComponent t{};
	t.mLocalScale = { 100.f, 100.f, 100.f };
	t.mLocalPosition = {0.f, 0.f, 500.f};

	mWorld->AddComponent<TransformComponent>(testEntity, t);
	mWorld->AddComponent<RenderComponent>(testEntity, sphereMesh, materials);

	Entity testlight = mWorld->CreateEntity();

	LightComponent l{};
	l.mLightInfo.Position = {Vec3(0, 1000, 500)};
	l.mLightInfo.Color.Ambient = { Vec3(0.1f, 0.1f, 0.1f) };
	l.mLightInfo.Color.Diffuse = { Vec3(1.f, 1.f, 1.f) };
	l.mLightInfo.Color.Specular = { Vec3(0.1f, 0.1f, 0.1f) };
	l.SetLightType(LIGHT_TYPE::DIRECTIONAL_LIGHT) ;
	l.SetLightDirection(Vec3(0, -1, 1.f));

	mWorld->AddComponent<LightComponent>(testlight, l);

	TransformComponent t2{};
	t2.mLocalPosition = { 0, 1000, 500 };
	Vec3 a = Vec3(0, -1.f, 1.f);
	a.Normalize();
	t2.LookAt (a);

	mWorld->AddComponent<TransformComponent>(testlight, t2);

	CameraComponent t3{};
		t3.SetScale(1.f);
		t3.SetFar(10000.f);
		t3.SetWidth(4096);
		t3.SetHeight(4096);


	mWorld->AddComponent<CameraComponent>(testlight,t3);
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


//
//
//
//void Scene::AddGameObject(shared_ptr<GameObject> gameObject)
//{
//	if (gameObject->GetCamera() != nullptr)
//	{
//		_cameras.push_back(gameObject->GetCamera());
//	}
//	else if (gameObject->GetLight() != nullptr)
//	{
//		_lights.push_back(gameObject->GetLight());
//	}
//
//
//	_gameObjects.push_back(gameObject);
//}
//
//void Scene::RemoveGameObject(shared_ptr<GameObject> gameObject)
//{
//
//	if (gameObject->GetCamera())
//	{
//		auto findIt = std::find(_cameras.begin(), _cameras.end(), gameObject->GetCamera());
//		if (findIt != _cameras.end())
//			_cameras.erase(findIt);
//	}
//	else if (gameObject->GetLight())
//	{
//		auto findIt = std::find(_lights.begin(), _lights.end(), gameObject->GetLight());
//		if (findIt != _lights.end())
//			_lights.erase(findIt);
//	}
//
//	auto findIt = std::find(_gameObjects.begin(), _gameObjects.end(), gameObject);
//	if (findIt != _gameObjects.end())
//	{
//		_gameObjects.erase(findIt);
//	}
//}
//
