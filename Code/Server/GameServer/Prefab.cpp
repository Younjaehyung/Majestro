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
#include "ColliderComponent.h"
#include "EnemyComponent.h"
#include "BulletComponent.h"
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
  t.mLocalScale = {1.f, 1.f, 1.f};

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

  Vec3 half{ 30,100,30 };
  Vec3 center{ 0,50,0 };
  world->AddComponent<BoxColliderComponent>(mEntityID, half, center);
  world->AddComponent<MovableComponent>(mEntityID);

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
  bt.mLocalScale = Vec3(100.f,100.f,100.f);

  //bt.mLocalPosition = Vec3(-0.5 * 505 * 100, -27.6f, -0.5 * 505 * 100);
  bt.mLocalPosition = Vec3(-0.5f * 504.f * 100.f, -27.6f, -0.5f * 504.f * 100.f);
  world->AddComponent<TransformComponent>(mEntityID, bt);

  auto heightField = std::make_shared<HeightField>();
  heightField->LoadHeightFieldFromPng16("../Resources/Texture/T_Height.png");
	
  
  // Add<HeightField>(L"TerrainHeightField", heightField);
  // Add<HeightField>(L"TerrainHeightField", heightField);

  TerrainComponent &terrainc =
      world->AddComponent<TerrainComponent>(mEntityID, 504, 504, heightField);
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

int EnemyPrefab::mSpawnCount = 0;
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

	TransformComponent t{};

	float n = 10;
	float i = (float)(mSpawnCount/10)*n , j = (float)(mSpawnCount % 10) * n, k;
	
	
	t.mLocalPosition = { i * n, 0, j * n };

	world->AddComponent<TransformComponent>(mEntityID, t);
	world->AddComponent<GravityComponent>(mEntityID);
	world->AddComponent<EnemyComponent>(mEntityID);
	world->AddComponent<EnemyMovementComponent>(mEntityID);
	world->AddComponent<BoxColliderComponent>(mEntityID);
	world->AddComponent<MovableComponent>(mEntityID);

	auto& w =
		world->AddComponent<NetEntityComponent>(mEntityID, world, mEntityID);
	w.mSessionId = ctx.SessionId;


	mSpawnCount++;

	return mEntityID;
}

BulletPrefab::BulletPrefab(World* world)
{
	mEntityID = Build(world, InputCommand{});
}

BulletPrefab::~BulletPrefab()
{
}

Entity BulletPrefab::Build(World* world, const InputCommand& ctx)
{
	Entity entity = world->CreateEntity();

	TransformComponent t{};
	t.mLocalPosition = { 0.f, 100.f, 0.f };
	t.mLocalScale = { 0.25f, 0.25f, 0.25f };

	world->AddComponent<TransformComponent>(entity, t);
	world->AddComponent<MovableComponent>(entity);
	world->AddComponent<BoxColliderComponent>(entity, Vec3(3.f, 3.f, 3.f));

	auto& bullet = world->AddComponent<BulletComponent>(entity);
	bullet.Activate(BulletType::Default, 0, 0, 0, Vec3::Forward, 60.0f, 2.0f, 10.0f);
	bullet.Deactivate(); // 풀에 넣기 위해 초기 상태는 비활성

	auto& net = world->AddComponent<NetEntityComponent>(entity, world, entity);
	net.mSessionId = ctx.SessionId;

	return entity;
}