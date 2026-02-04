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
	
	mWorld->Initialize();
}

void Scene::Update(float deltaTime)
{
	mWorld->Update(deltaTime);
}



