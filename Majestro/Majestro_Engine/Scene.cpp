#include "pch.h"
#include "Scene.h"
#include "Engine.h"
#include "RenderManager.h"
#include "World.h"
#include "Component.h"
#include "TagComponent.h"

#include "Prefab.h"
//#include "Camera.h"
//
//#include "ConstantBuffer.h"
//#include "Light.h"
//#include "Resources.h"


void Scene::Initialize()
{
	Entity testCamera =  mWorld->CreateEntity();
	mWorld->AddComponent<MainCameraComponent>(testCamera);

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
