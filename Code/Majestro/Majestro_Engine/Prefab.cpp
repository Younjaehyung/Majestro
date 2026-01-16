#include "pch.h"
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
#include "EnemyComponent.h"
#include "AnimationComponent.h"
#include "TerrainComponent.h"
#include "UITransformComponent.h"
#include "UISpriteComponent.h"
#include "BeatComponent.h"
#include "GravityComponent.h"
#include "MovementComponent.h"
#include "VfxComponent.h"
#include "NetEntityComponent.h"

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
	shared_ptr<Mesh> phereMesh = RESOURCEMANAGER.Get<Mesh>(L"SM_Rudwig_Body");

	std::vector<shared_ptr<Material>> material2s;

	//FBX File's Material [Nameing Convention : (filename)_(0~3)]
	shared_ptr<Material> material2 = RESOURCEMANAGER.Get<Material>(L"Anim_Rudwig_Idle0");


	material2s.push_back(material2);
	t.mLocalPosition = { 0.f, 0.f, 10.f };
	t.mLocalScale = { 10.f, 10.f, 10.f };

	//FBX File's Animation [Naming Convention : Anim_(Name)_(Animationtype)]
	vector<shared_ptr<Animator>> anmators0;
	anmators0.push_back(RESOURCEMANAGER.Get<Animator>(L"Anim_Rudwig_Idle"));
	anmators0.push_back(RESOURCEMANAGER.Get<Animator>(L"Anim_Rudwig_Walk"));
	anmators0.push_back(RESOURCEMANAGER.Get<Animator>(L"Anim_Rudwig_Run"));
	anmators0.push_back(RESOURCEMANAGER.Get<Animator>(L"Anim_Rudwig_Jump"));
	anmators0.push_back(RESOURCEMANAGER.Get<Animator>(L"Anim_Rudwig_Fall"));
	anmators0.push_back(RESOURCEMANAGER.Get<Animator>(L"Anim_Rudwig_Land"));
	anmators0.push_back(RESOURCEMANAGER.Get<Animator>(L"Anim_Rudwig_Run"));//dash


	world->AddComponent<ControllerComponent>(mEntityID, t);
	world->AddComponent<MainPlayerComponent>(mEntityID, "../Resources/Json/TestJson.json", anmators0);
	world->AddComponent<TransformComponent>(mEntityID, t);
	world->AddComponent<RenderComponent>(mEntityID, phereMesh, material2s);
	world->AddComponent<AnimationComponent>(mEntityID, anmators0);
	world->AddComponent<BeatComponent>(mEntityID);
	world->AddComponent<GravityComponent>(mEntityID);
	world->AddComponent<PlayerMovementComponent>(mEntityID);
	world->AddComponent<NetEntityComponent>(mEntityID);

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

	//FBX File's Mesh [Naming Convention : SM_(Meshname)_(parts)]
	shared_ptr<Mesh> phereMesh = RESOURCEMANAGER.Get<Mesh>(L"SM_Rudwig_Body");

	std::vector<shared_ptr<Material>> material2s;

	//FBX File's Material [Nameing Convention : (filename)_(0~3)]
	shared_ptr<Material> material2 = RESOURCEMANAGER.Get<Material>(L"Anim_Rudwig_Idle0");


	material2s.push_back(material2);
	t.mLocalPosition = { 0.f, 0.f, 10.f };
	t.mLocalScale = { 10.f, 10.f, 10.f };

	//FBX File's Animation [Naming Convention : Anim_(Name)_(Animationtype)]
	vector<shared_ptr<Animator>> anmators0;
	anmators0.push_back(RESOURCEMANAGER.Get<Animator>(L"Anim_Rudwig_Idle"));
	anmators0.push_back(RESOURCEMANAGER.Get<Animator>(L"Anim_Rudwig_Walk"));
	anmators0.push_back(RESOURCEMANAGER.Get<Animator>(L"Anim_Rudwig_Run"));
	anmators0.push_back(RESOURCEMANAGER.Get<Animator>(L"Anim_Rudwig_Jump"));
	anmators0.push_back(RESOURCEMANAGER.Get<Animator>(L"Anim_Rudwig_Run"));//dash


	world->AddComponent<ControllerComponent>(mEntityID, t);
	world->AddComponent<MainPlayerComponent>(mEntityID, "../Resources/Json/TestJson.json", anmators0);
	world->AddComponent<TransformComponent>(mEntityID, t);
	world->AddComponent<RenderComponent>(mEntityID, phereMesh, material2s);
	world->AddComponent<AnimationComponent>(mEntityID, anmators0);
	world->AddComponent<BeatComponent>(mEntityID);
	world->AddComponent<GravityComponent>(mEntityID);
	world->AddComponent<PlayerMovementComponent>(mEntityID);
	world->AddComponent<NetEntityComponent>(mEntityID);

	return mEntityID;
}

EnemyPrefab::EnemyPrefab(World* world)
{
	mEntityID = world->CreateEntity();

	shared_ptr<Mesh> phereMesh = RESOURCEMANAGER.Get<Mesh>(L"NoteHog_mBody");
	std::vector<shared_ptr<Material>> material2s;


	shared_ptr<Material> material2 = RESOURCEMANAGER.Get<Material>(L"NoteBoar_Run0");
	material2s.push_back(material2);
	TransformComponent t{};
	t.mLocalPosition = { 0.f, 0.f, 0.f };
	t.mLocalScale = { 0.5f, 0.5f, 0.5f };
	vector<shared_ptr<Animator>> anmators;
	anmators.push_back(RESOURCEMANAGER.Get<Animator>(L"NoteBoar_Rig|NoteBoar_aRun"));

	//mWorld->AddComponent<AnimationComponent>(osw, anmators);
	float i, j, k;
	float n = 10;
	for (i = -50; i < 50; i += 10.0f) {
		for (j = -50; j < 50; j += 10.0f) {
			//for (k = -50; k < 50; k += 10.0f) {
			Entity mEntityID = world->CreateEntity();
			t.mLocalPosition = { i * n, 0, j * n };


			world->AddComponent<TransformComponent>(mEntityID, t);
			world->AddComponent<RenderComponent>(mEntityID, phereMesh, material2s);
			world->AddComponent<GravityComponent>(mEntityID);
			world->AddComponent<AnimationComponent>(mEntityID, anmators);
			world->AddComponent<EnemyComponent>(mEntityID);
			world->AddComponent<EnemyMovementComponent>(mEntityID);
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
	Entity testCamera = world->CreateEntity();
	world->AddComponent<MainCameraComponent>(testCamera);
	world->AddComponent<CameraComponent>(testCamera);
	world->AddComponent<TransformComponent>(testCamera, t);
	world->AddComponent<CameraTypeComponent>(testCamera, mEntityID.GetID(), THREE_FPS);

	//FBX File's Mesh [Naming Convention : SM_(Meshname)_(parts)]
	shared_ptr<Mesh> phereMesh = RESOURCEMANAGER.Get<Mesh>(L"SM_Rudwig_Body");

	std::vector<shared_ptr<Material>> material2s;

	//FBX File's Material [Nameing Convention : (filename)_(0~3)]
	shared_ptr<Material> material2 = RESOURCEMANAGER.Get<Material>(L"Anim_Rudwig_Idle0");


	material2s.push_back(material2);
	t.mLocalPosition = { 0.f, 0.f, 10.f };
	t.mLocalScale = { 10.f, 10.f, 10.f };

	//FBX File's Animation [Naming Convention : Anim_(Name)_(Animationtype)]
	vector<shared_ptr<Animator>> anmators0;
	anmators0.push_back(RESOURCEMANAGER.Get<Animator>(L"Anim_Rudwig_Idle"));
	anmators0.push_back(RESOURCEMANAGER.Get<Animator>(L"Anim_Rudwig_Walk"));
	anmators0.push_back(RESOURCEMANAGER.Get<Animator>(L"Anim_Rudwig_Run"));
	anmators0.push_back(RESOURCEMANAGER.Get<Animator>(L"Anim_Rudwig_Jump"));
	anmators0.push_back(RESOURCEMANAGER.Get<Animator>(L"Anim_Rudwig_Run"));//dash


	world->AddComponent<ControllerComponent>(mEntityID, t);
	world->AddComponent<MainPlayerComponent>(mEntityID, "../Resources/Json/TestJson.json", anmators0);
	world->AddComponent<TransformComponent>(mEntityID, t);
	world->AddComponent<RenderComponent>(mEntityID, phereMesh, material2s);
	world->AddComponent<AnimationComponent>(mEntityID, anmators0);
	world->AddComponent<BeatComponent>(mEntityID);
	world->AddComponent<GravityComponent>(mEntityID);
	world->AddComponent<PlayerMovementComponent>(mEntityID);
	world->AddComponent<NetEntityComponent>(mEntityID);

	return mEntityID;
}

SkyBoxPrefab::SkyBoxPrefab(World* world)
{

	mEntityID = world->CreateEntity();
	TransformComponent bt{};


	shared_ptr<Mesh> skyBoxMesh = RESOURCEMANAGER.Get<Mesh>(L"Sphere");

	// 빌보드 머티리얼
	shared_ptr<Material> skyBoxMat = RESOURCEMANAGER.Get<Material>(L"Skybox");
	std::vector<shared_ptr<Material>> materials;
	materials.push_back(skyBoxMat);

	world->AddComponent<TransformComponent>(mEntityID, bt);
	RenderComponent& render = world->AddComponent<RenderComponent>(mEntityID, skyBoxMesh, materials);
	render.mCheckFrustum = false;

}

SkyBoxPrefab::~SkyBoxPrefab()
{
}

TerrainPrefab::TerrainPrefab(World* world)
{
	mEntityID = world->CreateEntity();
	TransformComponent bt{};
	bt.mLocalScale = (Vec3(30.f, 250.f, 30.f));
	bt.mLocalPosition = Vec3(-150.f, -70.f, -150.f);

	shared_ptr<Mesh> skyBoxMesh = RESOURCEMANAGER.LoadTerrainMesh(64, 64);

	// 빌보드 머티리얼(
	shared_ptr<Material> heightMap = RESOURCEMANAGER.Get<Material>(L"Terrain");
	std::vector<shared_ptr<Material>> materials;
	materials.push_back(heightMap);

	world->AddComponent<TransformComponent>(mEntityID, bt);
	TerrainComponent& terrainc = world->AddComponent<TerrainComponent>(mEntityID, 64, 64, heightMap);
	terrainc.mTerrainWorldPosition = bt.mLocalPosition;
	terrainc.mTerrainWorldScale = bt.mLocalScale;

	RenderComponent& render = world->AddComponent<RenderComponent>(mEntityID, skyBoxMesh, materials);
	render.mCheckFrustum = false;

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

	shared_ptr<Mesh> skyBoxMesh = RESOURCEMANAGER.LoadTerrainMesh(64, 64);

	// 빌보드 머티리얼(
	shared_ptr<Material> heightMap = RESOURCEMANAGER.Get<Material>(L"Terrain");
	std::vector<shared_ptr<Material>> materials;
	materials.push_back(heightMap);

	world->AddComponent<TransformComponent>(mEntityID, bt);
	TerrainComponent& terrainc = world->AddComponent<TerrainComponent>(mEntityID, 64, 64, heightMap);
	terrainc.mTerrainWorldPosition = bt.mLocalPosition;
	terrainc.mTerrainWorldScale = bt.mLocalScale;

	RenderComponent& render = world->AddComponent<RenderComponent>(mEntityID, skyBoxMesh, materials);
	render.mCheckFrustum = false;

	return mEntityID;
}

DirLightPrefab::DirLightPrefab(World* world)
{
	LightComponent l{};
	l.mLightInfo.Position = { Vec3(0, 1000, 500) };
	l.mLightInfo.Color.Ambient = { Vec3(0.1f, 0.1f, 0.1f) };
	l.mLightInfo.Color.Diffuse = { Vec3(1.f, 1.f, 1.f) };
	l.mLightInfo.Color.Specular = { Vec3(0.1f, 0.1f, 0.1f) };
	l.SetLightDirection(Vec3(0, -1, 1.f));
	mEntityID = LightFactory::CreateLight(world, LIGHT_TYPE::DIRECTIONAL_LIGHT, l);
}

DirLightPrefab::~DirLightPrefab()
{
}
