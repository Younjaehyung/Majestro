#include "pch.h"
#include "Prefab.h"
#include "ResourceManager.h"
#include "World.h"
#include "Component.h"
#include "TransformComponent.h"
#include "CameraComponent.h"
#include "LightComponent.h"
#include "TagComponent.h"
#include "PlayerComponent.h"
#include "TerrainComponent.h"
#include "BeatComponent.h"
#include "GravityComponent.h"
#include "MovementComponent.h"
#include "InputComponent.h"
#include "NetEntityComponent.h"
#include "HeightField.h"


Prefab::Prefab() : Object(OBJECT_TYPE::PREFAB)
{
}

Prefab::~Prefab()
{
}


PlayerPrefab::PlayerPrefab(World* world)
{
	mEntityID = world->CreateEntity();

	TransformComponent t{};
	Entity testCamera = world->CreateEntity();
	world->AddComponent<MainCameraComponent>(testCamera);
	world->AddComponent<CameraComponent>(testCamera);
	world->AddComponent<TransformComponent>(testCamera, t);
	world->AddComponent<CameraTypeComponent>(testCamera, mEntityID.GetID(), THREE_FPS);

	//FBX File's Mesh [Naming Convention : SM_(Meshname)_(parts)]
	


	world->AddComponent<ControllerComponent>(mEntityID, t);
	world->AddComponent<InputComponent>(mEntityID);
	world->AddComponent<MainPlayerComponent>(mEntityID, "../Resources/Json/TestJson.json");
	world->AddComponent<TransformComponent>(mEntityID, t);
	world->AddComponent<BeatComponent>(mEntityID);
	world->AddComponent<GravityComponent>(mEntityID);
	world->AddComponent<PlayerMovementComponent>(mEntityID);
	world->AddComponent<NetEntityComponent>(mEntityID, world, mEntityID);

}

PlayerPrefab::~PlayerPrefab()
{
}

Entity PlayerPrefab::Build(World* world, const InputCommand& ctx)
{
	Entity mEntityID = world->CreateEntity();

	TransformComponent t{};
	Entity testCamera = world->CreateEntity();
	world->AddComponent<MainCameraComponent>(testCamera);
	world->AddComponent<CameraComponent>(testCamera);
	world->AddComponent<TransformComponent>(testCamera, t);
	world->AddComponent<CameraTypeComponent>(testCamera, mEntityID.GetID(), THREE_FPS);

	t.mLocalPosition = { 0.f, 0.f, 10.f };
	t.mLocalScale = { 10.f, 10.f, 10.f };

	
	world->AddComponent<ControllerComponent>(mEntityID, t);
	world->AddComponent<MainPlayerComponent>(mEntityID, "../Resources/Json/TestJson.json");
	world->AddComponent<TransformComponent>(mEntityID, t);

	world->AddComponent<BeatComponent>(mEntityID);
	world->AddComponent<GravityComponent>(mEntityID);
	world->AddComponent<PlayerMovementComponent>(mEntityID);
	world->AddComponent<InputComponent>(mEntityID);
	auto& w = world->AddComponent<NetEntityComponent>(mEntityID,world,mEntityID);
	w.mSessionId = ctx.SessionId;
	return mEntityID;
}

SkyBoxPrefab::SkyBoxPrefab(World* world)
{

	mEntityID = world->CreateEntity();
	TransformComponent bt{};




	world->AddComponent<TransformComponent>(mEntityID, bt);


}

SkyBoxPrefab::~SkyBoxPrefab()
{
}

TerrainPrefab::TerrainPrefab(World* world)
{
	Entity mEntityID = world->CreateEntity();
	TransformComponent bt{};
	bt.mLocalScale = (Vec3(30.f, 250.f, 30.f));
	//bt.mLocalPosition = Vec3(-150.f, -70.f, -150.f);


	world->AddComponent<TransformComponent>(mEntityID, bt);

	auto heightField = std::make_shared<HeightField>();
	heightField->LoadHeightFieldFromRaw16("../Resources/Texture/height.raw", 64, 64, true);
	//Add<HeightField>(L"TerrainHeightField", heightField);

	TerrainComponent& terrainc = world->AddComponent<TerrainComponent>(mEntityID, 64, 64, heightField);
	terrainc.mTerrainWorldPosition = bt.mLocalPosition;
	terrainc.mTerrainWorldScale = bt.mLocalScale;
	terrainc.mTerrainParams.HeightMapResolution = Vec2(64.f, 64.f);
	terrainc.mTerrainParams.TileCountX = 16;
	terrainc.mTerrainParams.TileCountZ = 16;
	terrainc.mTerrainParams.MaxTessLevel = 5.f;
	terrainc.mTerrainParams.MinMaxTessDistance = Vec2(50.f, 300.f);
	
}

TerrainPrefab::~TerrainPrefab()
{
}

Entity TerrainPrefab::Build(World* world, const InputCommand& ctx)
{

	Entity mEntityID = world->CreateEntity();
	TransformComponent bt{};
	bt.mLocalScale = (Vec3(30.f, 250.f, 30.f));
	bt.mLocalPosition = Vec3(-150.f, -70.f, -150.f);


	world->AddComponent<TransformComponent>(mEntityID, bt);
	
	auto heightField = std::make_shared<HeightField>();
	heightField->LoadHeightFieldFromRaw16("height.raw", 2048, 2048, true);
	//Add<HeightField>(L"TerrainHeightField", heightField);

	TerrainComponent& terrainc = world->AddComponent<TerrainComponent>(mEntityID,2048, 2048, heightField);
	terrainc.mTerrainWorldPosition = bt.mLocalPosition;
	terrainc.mTerrainWorldScale = bt.mLocalScale;


	return mEntityID;
}
