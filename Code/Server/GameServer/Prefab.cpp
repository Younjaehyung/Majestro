#include "pch.h"
#include "Prefab.h"
#include "BeatComponent.h"
#include "CameraComponent.h"
#include "Component.h"
#include "GravityComponent.h"
#include "HeightField.h"
#include "InputComponent.h"
#include "LightComponent.h"
#include "MovementComponent.h"
#include "NetEntityComponent.h"
#include "PlayerComponent.h"
#include "ResourceManager.h"
#include "TagComponent.h"
#include "TerrainComponent.h"
#include "TransformComponent.h"
#include "BoxColliderComponent.h"
#include "EnemyComponent.h"
#include "World.h"


Prefab::Prefab() : Object(OBJECT_TYPE::PREFAB) {}

Prefab::~Prefab() {}

PlayerPrefab::PlayerPrefab(World *world) {
  mEntityID = world->CreateEntity();

  TransformComponent t{};
  Entity testCamera = world->CreateEntity();
  world->AddComponent<MainCameraComponent>(testCamera);
  world->AddComponent<CameraComponent>(testCamera);
  world->AddComponent<TransformComponent>(testCamera, t);
  world->AddComponent<CameraTypeComponent>(testCamera, mEntityID.GetID(),
                                           THREE_FPS);

  // FBX File's Mesh [Naming Convention : SM_(Meshname)_(parts)]

  world->AddComponent<ControllerComponent>(mEntityID, t);
  world->AddComponent<InputComponent>(mEntityID);
  world->AddComponent<MainPlayerComponent>(mEntityID,
                                           "../Resources/Json/TestJson.json");
  world->AddComponent<TransformComponent>(mEntityID, t);
  world->AddComponent<BeatComponent>(mEntityID);
  world->AddComponent<GravityComponent>(mEntityID);
  world->AddComponent<PlayerMovementComponent>(mEntityID);
 // world->AddComponent<NetEntityComponent>(mEntityID, world, mEntityID);
}

PlayerPrefab::~PlayerPrefab() {}

Entity PlayerPrefab::Build(World *world, const InputCommand &ctx) {
  Entity mEntityID = world->CreateEntity();

  TransformComponent t{};
  Entity testCamera = world->CreateEntity();
  world->AddComponent<MainCameraComponent>(testCamera);
  world->AddComponent<CameraComponent>(testCamera);
  world->AddComponent<TransformComponent>(testCamera, t);
  world->AddComponent<CameraTypeComponent>(testCamera, mEntityID.GetID(),
                                           THREE_FPS);

  t.mLocalPosition = {0.f, 0.f, 10.f};
  t.mLocalScale = {10.f, 10.f, 10.f};

  world->AddComponent<ControllerComponent>(mEntityID, t);
  world->AddComponent<MainPlayerComponent>(mEntityID,
                                           "../Resources/Json/TestJson.json");
  world->AddComponent<TransformComponent>(mEntityID, t);

  world->AddComponent<BeatComponent>(mEntityID);
  world->AddComponent<GravityComponent>(mEntityID);
  world->AddComponent<PlayerMovementComponent>(mEntityID);
  world->AddComponent<InputComponent>(mEntityID);
  auto &w =
      world->AddComponent<NetEntityComponent>(mEntityID, world, mEntityID);
  w.mSessionId = ctx.SessionId;

  Vec3 half{ 10,10,10 };
  Vec3 center{ 0,10,0 };
  world->AddComponent<BoxColliderComponent>(mEntityID, half, center);

  return mEntityID;
}

SkyBoxPrefab::SkyBoxPrefab(World *world) {

  mEntityID = world->CreateEntity();
  TransformComponent bt{};

  world->AddComponent<TransformComponent>(mEntityID, bt);
}

SkyBoxPrefab::~SkyBoxPrefab() {}

TerrainPrefab::TerrainPrefab(World *world) {
  Entity mEntityID = world->CreateEntity();
  TransformComponent bt{};
  bt.mLocalScale = (Vec3(1, 800.f, 1));
  bt.mLocalPosition = Vec3(-150.f, -70.f, -150.f);

  world->AddComponent<TransformComponent>(mEntityID, bt);

  auto heightField = std::make_shared<HeightField>();
  heightField->LoadHeightFieldFromPng16("../Resources/Texture/Heightmap_R16.png");
  // Add<HeightField>(L"TerrainHeightField", heightField);
  // Add<HeightField>(L"TerrainHeightField", heightField);

  TerrainComponent &terrainc =
      world->AddComponent<TerrainComponent>(mEntityID, 1000, 1000, heightField);
  terrainc.mTerrainWorldPosition = bt.mLocalPosition;
  terrainc.mTerrainWorldScale = bt.mLocalScale;
}

TerrainPrefab::~TerrainPrefab() {}

Entity TerrainPrefab::Build(World *world, const InputCommand &ctx) {

  Entity mEntityID = world->CreateEntity();
  TransformComponent bt{};
  bt.mLocalScale = (Vec3(30.f, 250.f, 30.f));
  bt.mLocalPosition = Vec3(-150.f, -70.f, -150.f);

  world->AddComponent<TransformComponent>(mEntityID, bt);

  auto heightField = std::make_shared<HeightField>();
  heightField->LoadHeightFieldFromPng16("height.raw");
  // Add<HeightField>(L"TerrainHeightField", heightField);

  TerrainComponent &terrainc =
      world->AddComponent<TerrainComponent>(mEntityID, 64, 64, heightField);
  terrainc.mTerrainWorldPosition = bt.mLocalPosition;
  terrainc.mTerrainWorldScale = bt.mLocalScale;

  return mEntityID;
}


EnemyPrefab::EnemyPrefab(World* world)
{
	mEntityID = world->CreateEntity();
	TransformComponent t{};
	t.mLocalPosition = { 0.f, 0.f, 0.f };
	t.mLocalScale = { 0.5f, 0.5f, 0.5f };


	//mWorld->AddComponent<AnimationComponent>(osw, anmators);
	float i, j, k;
	float n = 10;
	for (i = -50; i < 50; i += 10.0f) {
		for (j = -50; j < 50; j += 10.0f) {
			//for (k = -50; k < 50; k += 10.0f) {
			Entity mEntityID = world->CreateEntity();
			t.mLocalPosition = { i * n, 0, j * n };


			world->AddComponent<TransformComponent>(mEntityID, t);

			world->AddComponent<GravityComponent>(mEntityID);

			world->AddComponent<EnemyComponent>(mEntityID);
			world->AddComponent<EnemyMovementComponent>(mEntityID);
			world->AddComponent<BoxColliderComponent>(mEntityID);
			//}
		}

	}

}

EnemyPrefab::~EnemyPrefab()
{
}

Entity EnemyPrefab::Build(World* world, const InputCommand& ctx)
{
	Entity mEntityID = world->CreateEntity();

	return mEntityID;
}
