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
#include "NetTransformComponent.h"
#include "BoxColliderComponent.h"

Prefab::Prefab() : Object(OBJECT_TYPE::PREFAB)
{
}

Prefab::~Prefab()
{
}


PlayerPrefab::PlayerPrefab(World* world)
{
	mEntityID = world->CreateEntity();
	cout << "/////////////////////////////////////" << endl;
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
	anmators0.push_back(RESOURCEMANAGER.Get<Animator>(L"Anim_Rudwig_Attack_01"));//dash


	world->AddComponent<ControllerComponent>(mEntityID, t);
	world->AddComponent<MainPlayerComponent>(mEntityID, "../Resources/Json/TestJson.json", anmators0);
	world->AddComponent<TransformComponent>(mEntityID, t);
	world->AddComponent<RenderComponent>(mEntityID, phereMesh, material2s);
	world->AddComponent<AnimationComponent>(mEntityID, anmators0);
	world->AddComponent<BeatComponent>(mEntityID);
	world->AddComponent<GravityComponent>(mEntityID);
	world->AddComponent<PlayerMovementComponent>(mEntityID);
	world->AddComponent<NetEntityComponent>(mEntityID);

	Vec3 half{ 10,10,10 };
	Vec3 center{ 0,10,0 };
	world->AddComponent<BoxColliderComponent>(mEntityID,half,center);

}

PlayerPrefab::~PlayerPrefab()
{
}

Entity PlayerPrefab::Build(World* world, const InputCommand& ctx)
{
	Entity mEntityID = world->CreateEntity();

	
	
	//FBX File's Mesh [Naming Convention : SM_(Meshname)_(parts)]
	shared_ptr<Mesh> phereMesh;

	//FBX File's Material [Nameing Convention : (filename)_(0~3)]
	shared_ptr<Material> material2;

	std::vector<shared_ptr<Material>> material2s;
	
	

	//FBX File's Animation [Naming Convention : Anim_(Name)_(Animationtype)]
	vector<shared_ptr<Animator>> anmators0;


	//world->AddComponent<ControllerComponent>(mEntityID, t);

	switch (ctx.ViewAs<S2C_SpawnPacekt>()->isPlayerType){// ctx.ViewAs<S2C_SpawnPacekt>()->isPlayerType) {

	case 0:

		phereMesh = RESOURCEMANAGER.Get<Mesh>(L"SM_Rudwig_Body");
		material2 = RESOURCEMANAGER.Get<Material>(L"Anim_Rudwig_Attack_010");
		material2s.push_back(material2);
		material2 = RESOURCEMANAGER.Get<Material>(L"Anim_Rudwig_Attack_011");
		material2s.push_back(material2);
		anmators0.push_back(RESOURCEMANAGER.Get<Animator>(L"Anim_Rudwig_Idle"));
		anmators0.push_back(RESOURCEMANAGER.Get<Animator>(L"Anim_Rudwig_Walk"));/*Anim_Rudwig_Walk*/
		anmators0.push_back(RESOURCEMANAGER.Get<Animator>(L"Anim_Rudwig_Run"));
		anmators0.push_back(RESOURCEMANAGER.Get<Animator>(L"Anim_Rudwig_Jump"));
		anmators0.push_back(RESOURCEMANAGER.Get<Animator>(L"Anim_Rudwig_Fall"));
		anmators0.push_back(RESOURCEMANAGER.Get<Animator>(L"Anim_Rudwig_Land"));
		anmators0.push_back(RESOURCEMANAGER.Get<Animator>(L"Anim_Rudwig_Run"));//dash
		anmators0.push_back(RESOURCEMANAGER.Get<Animator>(L"Anim_Rudwig_Attack_01"));//attack1
		anmators0.push_back(RESOURCEMANAGER.Get<Animator>(L"Anim_Rudwig_Attack_01"));//attack2
		anmators0.push_back(RESOURCEMANAGER.Get<Animator>(L"Anim_Rudwig_Attack_01"));//skill1
		anmators0.push_back(RESOURCEMANAGER.Get<Animator>(L"Anim_Rudwig_Attack_01"));//skill2
		anmators0.push_back(RESOURCEMANAGER.Get<Animator>(L"Anim_Rudwig_Attack_01"));//special

		world->AddComponent<MainPlayerComponent>(mEntityID, "../Resources/Json/TestJson.json", anmators0, ctx.ViewAs<S2C_SpawnPacekt>()->isPlayerType);

		break;
	case 1:
		
		phereMesh = RESOURCEMANAGER.Get<Mesh>(L"SM_Ibanix_Body");
		material2 = RESOURCEMANAGER.Get<Material>(L"Anim_Ibanix_Idle0");
		material2s.push_back(material2);
		anmators0.push_back(RESOURCEMANAGER.Get<Animator>(L"Anim_Ibanix_Idle"));
		anmators0.push_back(RESOURCEMANAGER.Get<Animator>(L"Anim_Ibanix_Walk"));
		anmators0.push_back(RESOURCEMANAGER.Get<Animator>(L"Anim_Ibanix_Run"));
		anmators0.push_back(RESOURCEMANAGER.Get<Animator>(L"Anim_Ibanix_Jump"));
		anmators0.push_back(RESOURCEMANAGER.Get<Animator>(L"Anim_Ibanix_Fall"));
		anmators0.push_back(RESOURCEMANAGER.Get<Animator>(L"Anim_Ibanix_Land"));
		anmators0.push_back(RESOURCEMANAGER.Get<Animator>(L"Anim_Ibanix_Run"));//dash
		world->AddComponent<MainPlayerComponent>(mEntityID, "../Resources/Json/TestJson.json", anmators0, ctx.ViewAs<S2C_SpawnPacekt>()->isPlayerType);
		break;
	case 2:

		phereMesh = RESOURCEMANAGER.Get<Mesh>(L"SM_Fanthor_Body");
		material2 = RESOURCEMANAGER.Get<Material>(L"Anim_Fanthor_Idle0");
		material2s.push_back(material2);
		anmators0.push_back(RESOURCEMANAGER.Get<Animator>(L"Anim_Fanthor_Idle"));
		anmators0.push_back(RESOURCEMANAGER.Get<Animator>(L"Anim_Fanthor_Walk"));
		anmators0.push_back(RESOURCEMANAGER.Get<Animator>(L"Anim_Fanthor_Run"));
		anmators0.push_back(RESOURCEMANAGER.Get<Animator>(L"Anim_Fanthor_Jump"));
		anmators0.push_back(RESOURCEMANAGER.Get<Animator>(L"Anim_Fanthor_Fall"));
		anmators0.push_back(RESOURCEMANAGER.Get<Animator>(L"Anim_Fanthor_Land"));
		anmators0.push_back(RESOURCEMANAGER.Get<Animator>(L"Anim_Fanthor_Run"));//dash
		world->AddComponent<MainPlayerComponent>(mEntityID, "../Resources/Json/TestJson.json", anmators0, ctx.ViewAs<S2C_SpawnPacekt>()->isPlayerType);
		break;
	}

	TransformComponent t{};
	t.mLocalPosition = { 0.f, 0.f, 10.f };
	world->AddComponent<TransformComponent>(mEntityID, t);
	world->AddComponent<RenderComponent>(mEntityID, phereMesh, material2s);
	world->AddComponent<AnimationComponent>(mEntityID, anmators0);
	world->AddComponent<BeatComponent>(mEntityID);
	world->AddComponent<GravityComponent>(mEntityID);
	world->AddComponent<PlayerMovementComponent>(mEntityID);
	world->AddComponent<NetTransformComponent>(mEntityID);
	
	if(ctx.ViewAs<S2C_SpawnPacekt>()->isLocalPlayer == 1)
	{
		world->AddComponent<LocalPlayerComponent>(mEntityID);
		
		
		Entity testCamera = world->CreateEntity();
		world->AddComponent<MainCameraComponent>(testCamera);
		world->AddComponent<CameraComponent>(testCamera);
		world->AddComponent<TransformComponent>(testCamera, t);
		world->AddComponent<CameraTypeComponent>(testCamera, mEntityID.GetID(), THREE_FPS);
	}


	Vec3 half{ 10,10,10 };	
	Vec3 center{ 0,10,0 };
	world->AddComponent<BoxColliderComponent>(mEntityID, half, center);

	auto& netComp = world->AddComponent<NetEntityComponent>(mEntityID);
	netComp.mOwnerEntity = mEntityID;
	netComp.mNetEntityId = ctx.ViewAs<S2C_SpawnPacekt>()->netEntityId;
	world->NetIdBinding(netComp.mNetEntityId, mEntityID);
	
	std::cout << "Create Prefab" << std::endl;
	return mEntityID;
}

EnemyPrefab::EnemyPrefab(World* world)
{
	//mEntityID = world->CreateEntity();

	shared_ptr<Mesh> phereMesh = RESOURCEMANAGER.Get<Mesh>(L"SM_Noteboar_Body");
	std::vector<shared_ptr<Material>> material2s;


	shared_ptr<Material> material2 = RESOURCEMANAGER.Get<Material>(L"SK_NoteBoar_Run0");
	material2s.push_back(material2);
	TransformComponent t{};
	t.mLocalPosition = { 0.f, 0.f, 0.f };
	t.mLocalScale = { 0.5f, 0.5f, 0.5f };
	vector<shared_ptr<Animator>> anmators;
	anmators.push_back(RESOURCEMANAGER.Get<Animator>(L"SM_Noteboar_Body.001|Action"));

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
			world->AddComponent<BoxColliderComponent>(mEntityID);


			/*auto& netComp = world->AddComponent<NetEntityComponent>(mEntityID);
			netComp.mOwnerEntity = mEntityID;
			netComp.mNetEntityId = ctx.ViewAs<S2C_SpawnPacekt>()->netEntityId;
			world->NetIdBinding(netComp.mNetEntityId, mEntityID);*/


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

	shared_ptr<Mesh> phereMesh = RESOURCEMANAGER.Get<Mesh>(L"SM_Noteboar_Body");
	std::vector<shared_ptr<Material>> material2s;


	shared_ptr<Material> material2 = RESOURCEMANAGER.Get<Material>(L"SK_NoteBoar_Run0");
	material2s.push_back(material2);
	TransformComponent t{};
	t.mLocalPosition = { 0.f, 0.f, 0.f };
	t.mLocalScale = { 0.5f, 0.5f, 0.5f };
	vector<shared_ptr<Animator>> anmators;
	anmators.push_back(RESOURCEMANAGER.Get<Animator>(L"SM_Noteboar_Body.001|Action"));

	world->AddComponent<TransformComponent>(mEntityID, t);
	world->AddComponent<NetTransformComponent>(mEntityID);
	world->AddComponent<RenderComponent>(mEntityID, phereMesh, material2s);
	//world->AddComponent<GravityComponent>(mEntityID);
	world->AddComponent<AnimationComponent>(mEntityID, anmators);
	//world->AddComponent<EnemyComponent>(mEntityID);
	//world->AddComponent<EnemyMovementComponent>(mEntityID);
	world->AddComponent<BoxColliderComponent>(mEntityID);

	auto& netComp = world->AddComponent<NetEntityComponent>(mEntityID);
	netComp.mOwnerEntity = mEntityID;
	netComp.mNetEntityId = ctx.ViewAs<S2C_SpawnPacekt>()->netEntityId;
	world->NetIdBinding(netComp.mNetEntityId, mEntityID);


	

	return mEntityID;
}

SkyBoxPrefab::SkyBoxPrefab(World* world)
{

	mEntityID = world->CreateEntity();
	TransformComponent bt{};


	shared_ptr<Mesh> skyBoxMesh = RESOURCEMANAGER.Get<Mesh>(L"Sphere");

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
	bt.mLocalScale = Vec3(100, 100, 100);
	//bt.mLocalRotationE = Vec3(0, 90, 0);
	bt.mLocalPosition = Vec3(-0.5 * 504 * 100, -27.6f, -0.5 * 504 * 100);


	world->AddComponent<TransformComponent>(mEntityID, bt);

	// heightmap 512x512 => 타일 511x511


	shared_ptr<Mesh> terrain = RESOURCEMANAGER.LoadTerrainMesh(504, 504);
	shared_ptr<Material> heightMap = RESOURCEMANAGER.Get<Material>(L"Terrain");

	TerrainComponent& terrainc = world->AddComponent<TerrainComponent>(mEntityID, 504, 504, heightMap);
	terrainc.mTerrainWorldPosition = bt.mLocalPosition;
	terrainc.mTerrainWorldScale = bt.mLocalScale;

	std::vector<shared_ptr<Material>> materials{
		
		RESOURCEMANAGER.Get<Material>(L"Grass"),
		RESOURCEMANAGER.Get<Material>(L"Rock"),
		RESOURCEMANAGER.Get<Material>(L"Dirt"),
		//RESOURCEMANAGER.Get<Material>(L"SnowFootprints"),
		//RESOURCEMANAGER.Get<Material>(L"Soil_Mud") ,
		//RESOURCEMANAGER.Get<Material>(L"Asphalt")
	};

	RenderComponent& render = world->AddComponent<RenderComponent>(mEntityID, terrain, materials);
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
	l.mLightInfo.Position = { Vec3(0, 0, 0) };
	l.mLightInfo.Color.Ambient = { Vec3(0.1f, 0.1f, 0.1f) };
	l.mLightInfo.Color.Diffuse = { Vec3(1.f, 1.f, 1.f) };
	l.mLightInfo.Color.Specular = { Vec3(0.1f, 0.1f, 0.1f) };
	l.SetLightDirection(Vec3(0, -1, 1.f));
	mEntityID = LightFactory::CreateLight(world, LIGHT_TYPE::DIRECTIONAL_LIGHT, l);
}

DirLightPrefab::~DirLightPrefab()
{
}

BillboardPrefab::BillboardPrefab(World* world)
{
}

BillboardPrefab::~BillboardPrefab()
{
}
