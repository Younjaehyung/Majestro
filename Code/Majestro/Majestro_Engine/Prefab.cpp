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
#include "UIComponent.h"
#include "BeatComponent.h"
#include "GravityComponent.h"
#include "MovementComponent.h"
#include "VfxComponent.h"
#include "NetEntityComponent.h"
#include "NetTransformComponent.h"
#include "BoxColliderComponent.h"
#include "UITextComponent.h"
#include "BulletComponent.h"
#include "HealthComponent.h"
#include "ArmorComponent.h"


Prefab::Prefab() : Object(OBJECT_TYPE::PREFAB)
{
}

Prefab::~Prefab()
{
}

Entity PrefabFactory::BuildWorldMarkerPrefab(World* world, const InputCommand& ctx, const wchar_t* effectName, const Vec3& scale)
{
	Entity entity = world->CreateEntity();

	TransformComponent transform{};
	world->AddComponent<TransformComponent>(entity, transform);
	world->AddComponent<NetTransformComponent>(entity);

	auto& netComp = world->AddComponent<NetEntityComponent>(entity);
	netComp.mOwnerEntity = entity;

	if (const S2C_SpawnPacekt* spawnPacket = ctx.ViewAs<S2C_SpawnPacekt>())
	{
		netComp.mNetEntityId = spawnPacket->netEntityId;
		world->NetIdBinding(netComp.mNetEntityId, entity);
	}

	VfxComponent& vfxComp = world->AddComponent<VfxComponent>(entity);
	vfxComp.mVfx = RESOURCEMANAGER.Get<Vfx>(effectName);
	vfxComp.mIsLoop = true;
	vfxComp.mScale = scale;
	return entity;
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
	anmators0.push_back(RESOURCEMANAGER.Get<Animator>(L"Anim_Rudwig_fall"));
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
	world->AddComponent<HealthComponent>(mEntityID, 100, 100);
	world->AddComponent<UIHpBarComponent>(mEntityID, 180.f, mEntityID, Vec3(0.f, 200.f, 0.f), 20.f);

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

	switch (ctx.ViewAs<S2C_SpawnPacekt>()->Type) {// ctx.ViewAs<S2C_SpawnPacekt>()->isPlayerType) {

	case 0:
	{
		world->AddComponent<HealthComponent>(mEntityID, 150, 150);
		world->AddComponent<ArmorComponent>(mEntityID, 200, 0);

		phereMesh = RESOURCEMANAGER.Get<Mesh>(L"SM_Rudwig_Body");
		material2 = RESOURCEMANAGER.Get<Material>(L"Anim_Rudwig_Base0");
		
		material2s.push_back(material2);
		material2 = RESOURCEMANAGER.Get<Material>(L"Anim_Rudwig_Base1");
		material2s.push_back(material2);
		anmators0.push_back(RESOURCEMANAGER.Get<Animator>(L"Anim_Rudwig_Idle"));
		anmators0.push_back(RESOURCEMANAGER.Get<Animator>(L"Anim_Rudwig_Run"));
		anmators0.push_back(RESOURCEMANAGER.Get<Animator>(L"Anim_Rudwig_BackRun"));
		anmators0.push_back(RESOURCEMANAGER.Get<Animator>(L"Anim_Rudwig_RightRun"));
		anmators0.push_back(RESOURCEMANAGER.Get<Animator>(L"Anim_Rudwig_LeftRun"));
		//anmators0.push_back(RESOURCEMANAGER.Get<Animator>(L"Anim_Rudwig_Run"));
		anmators0.push_back(RESOURCEMANAGER.Get<Animator>(L"Anim_Rudwig_Jump"));
		anmators0.push_back(RESOURCEMANAGER.Get<Animator>(L"Anim_Rudwig_fall"));
		anmators0.push_back(RESOURCEMANAGER.Get<Animator>(L"Anim_Rudwig_Land"));
		anmators0.push_back(RESOURCEMANAGER.Get<Animator>(L"Anim_Rudwig_Skill_02"));//dash
		anmators0.push_back(RESOURCEMANAGER.Get<Animator>(L"Anim_Rudwig_Attack_01"));//attack1
		anmators0.push_back(RESOURCEMANAGER.Get<Animator>(L"Anim_Rudwig_Attack_01"));//attack2
		anmators0.push_back(RESOURCEMANAGER.Get<Animator>(L"Anim_Rudwig_Skill_01"));//skill1
		anmators0.push_back(RESOURCEMANAGER.Get<Animator>(L"Anim_Rudwig_Skill_02"));//skill2
		anmators0.push_back(RESOURCEMANAGER.Get<Animator>(L"Anim_Rudwig_Attack_01"));//special
		anmators0.push_back(RESOURCEMANAGER.Get<Animator>(L"Anim_Rudwig_Reload"));//special
		anmators0.push_back(RESOURCEMANAGER.Get<Animator>(L"Anim_Rudwig_Rhythm"));

		world->AddComponent<MainPlayerComponent>(mEntityID, "../Resources/Json/TestJson.json", anmators0, ctx.ViewAs<S2C_SpawnPacekt>()->Type);
	}
		break;
	case 1:
	{
		world->AddComponent<HealthComponent>(mEntityID, 100, 100);
		world->AddComponent<ArmorComponent>(mEntityID, 50, 0);

		phereMesh = RESOURCEMANAGER.Get<Mesh>(L"SM_Ibanix_Body");
		material2 = RESOURCEMANAGER.Get<Material>(L"Anim_Ibanix_Base0");
		material2s.push_back(material2);
		material2 = RESOURCEMANAGER.Get<Material>(L"Anim_Ibanix_Base1");
		material2s.push_back(material2);
		anmators0.push_back(RESOURCEMANAGER.Get<Animator>(L"Anim_Ibanix_Idle"));
		anmators0.push_back(RESOURCEMANAGER.Get<Animator>(L"Anim_Ibanix_Run")); //forward
		anmators0.push_back(RESOURCEMANAGER.Get<Animator>(L"Anim_Ibanix_BackRun")); //backword
		anmators0.push_back(RESOURCEMANAGER.Get<Animator>(L"Anim_Ibanix_RightRun")); //right
		anmators0.push_back(RESOURCEMANAGER.Get<Animator>(L"Anim_Ibanix_LeftRun")); //left
		//anmators0.push_back(RESOURCEMANAGER.Get<Animator>(L"Anim_Ibanix_Run"));
		anmators0.push_back(RESOURCEMANAGER.Get<Animator>(L"Anim_Ibanix_Jump"));
		anmators0.push_back(RESOURCEMANAGER.Get<Animator>(L"Anim_Ibanix_Fall"));
		anmators0.push_back(RESOURCEMANAGER.Get<Animator>(L"Anim_Ibanix_Land"));
		anmators0.push_back(RESOURCEMANAGER.Get<Animator>(L"Anim_Ibanix_Skill_02"));//dash
		anmators0.push_back(RESOURCEMANAGER.Get<Animator>(L"Anim_Ibanix_Attack_01"));//attack1
		anmators0.push_back(RESOURCEMANAGER.Get<Animator>(L"Anim_Ibanix_Attack_01"));//attack2
		anmators0.push_back(RESOURCEMANAGER.Get<Animator>(L"Anim_Ibanix_Skill_01"));//skill1
		anmators0.push_back(RESOURCEMANAGER.Get<Animator>(L"Anim_Ibanix_Skill_02"));//skill2
		anmators0.push_back(RESOURCEMANAGER.Get<Animator>(L"Anim_Ibanix_Attack_01"));//special
		anmators0.push_back(RESOURCEMANAGER.Get<Animator>(L"Anim_Ibanix_Reload"));
		anmators0.push_back(RESOURCEMANAGER.Get<Animator>(L"Anim_Ibanix_Rhythm"));
		world->AddComponent<MainPlayerComponent>(mEntityID, "../Resources/Json/TestJson.json", anmators0, ctx.ViewAs<S2C_SpawnPacekt>()->Type);
	}
		break;
	case 2:
	{
		world->AddComponent<HealthComponent>(mEntityID, 125, 125);
		world->AddComponent<ArmorComponent>(mEntityID, 50, 0);

		phereMesh = RESOURCEMANAGER.Get<Mesh>(L"SM_Fanthor_Body");
		material2 = RESOURCEMANAGER.Get<Material>(L"Anim_Fanthor_Base0");

		material2s.push_back(material2);
		material2 = RESOURCEMANAGER.Get<Material>(L"Anim_Fanthor_Base1");
		material2s.push_back(material2);
		/*material2 = RESOURCEMANAGER.Get<Material>(L"Anim_Fanthor_Idle0");
		material2s.push_back(material2);*/
		anmators0.push_back(RESOURCEMANAGER.Get<Animator>(L"Anim_Fanthor_Idle"));
		anmators0.push_back(RESOURCEMANAGER.Get<Animator>(L"Anim_Fanthor_Run"));
		anmators0.push_back(RESOURCEMANAGER.Get<Animator>(L"Anim_Fanthor_BackRun"));
		anmators0.push_back(RESOURCEMANAGER.Get<Animator>(L"Anim_Fanthor_RightRun"));
		anmators0.push_back(RESOURCEMANAGER.Get<Animator>(L"Anim_Fanthor_LeftRun"));
		//anmators0.push_back(RESOURCEMANAGER.Get<Animator>(L"Anim_Fanthor_Run"));
		anmators0.push_back(RESOURCEMANAGER.Get<Animator>(L"Anim_Fanthor_Jump"));
		anmators0.push_back(RESOURCEMANAGER.Get<Animator>(L"Anim_Fanthor_Fall"));
		anmators0.push_back(RESOURCEMANAGER.Get<Animator>(L"Anim_Fanthor_Land"));
		anmators0.push_back(RESOURCEMANAGER.Get<Animator>(L"Anim_Fanthor_Skill_02"));//dash
		anmators0.push_back(RESOURCEMANAGER.Get<Animator>(L"Anim_Fanthor_Attack_01"));//attack1
		anmators0.push_back(RESOURCEMANAGER.Get<Animator>(L"Anim_Fanthor_Attack_01"));//attack2
		anmators0.push_back(RESOURCEMANAGER.Get<Animator>(L"Anim_Fanthor_Skill_01"));//skill1
		anmators0.push_back(RESOURCEMANAGER.Get<Animator>(L"Anim_Fanthor_Skill_02"));//skill2
		anmators0.push_back(RESOURCEMANAGER.Get<Animator>(L"Anim_Fanthor_Attack_01"));//special
		anmators0.push_back(RESOURCEMANAGER.Get<Animator>(L"Anim_Fanthor_Reload"));
		anmators0.push_back(RESOURCEMANAGER.Get<Animator>(L"Anim_Fanthor_Rhythm"));
		world->AddComponent<MainPlayerComponent>(mEntityID, "../Resources/Json/TestJson.json", anmators0, ctx.ViewAs<S2C_SpawnPacekt>()->Type);
	}
		break;
	}



	TransformComponent t{};
	t.mLocalPosition = { 0.f, 0.f, 10.f };
	world->AddComponent<TransformComponent>(mEntityID, t);
	RenderComponent& render = world->AddComponent<RenderComponent>(mEntityID, phereMesh, material2s);
	world->AddComponent<AnimationComponent>(mEntityID, anmators0);
	world->AddComponent<BeatComponent>(mEntityID);
	world->AddComponent<GravityComponent>(mEntityID);
	world->AddComponent<PlayerMovementComponent>(mEntityID);
	world->AddComponent<NetTransformComponent>(mEntityID);
	
	if(ctx.ViewAs<S2C_SpawnPacekt>()->isLocalPlayer == 1)
	{
		world->AddComponent<LocalPlayerComponent>(mEntityID);
		render.mCheckFrustum = false;
		
		Entity testCamera = world->CreateEntity();
		world->AddComponent<MainCameraComponent>(testCamera);
		world->AddComponent<CameraComponent>(testCamera);
		world->AddComponent<TransformComponent>(testCamera, t);
		world->AddComponent<CameraTypeComponent>(testCamera, mEntityID.GetID(), THREE_FPS);

		HUDPortraitPrefab::HUDPortraitPrefab(world, ctx.ViewAs<S2C_SpawnPacekt>()->Type);
		HUDWeaponPrefab::HUDWeaponPrefab(world, ctx.ViewAs<S2C_SpawnPacekt>()->Type, mEntityID);
		HUDMusicPrefab::HUDMusicPrefab(world, ctx.ViewAs<S2C_SpawnPacekt>()->Type, mEntityID);
		HUDHPBarPrefab::HUDHPBarPrefab(world, ctx.ViewAs<S2C_SpawnPacekt>()->Type, mEntityID);
	}


	Vec3 half{ 30,100,30 };	
	Vec3 center{ 0,50,0 };
	world->AddComponent<BoxColliderComponent>(mEntityID, half, center);
	world->AddComponent<UIHpBarComponent>(mEntityID, 180.f, mEntityID, Vec3(0.f, 200.f, 0.f), 20.f, L"HPBAR_RUDWIG", L"HPBAR_IBANIX");

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

	TransformComponent t{};
	t.mLocalPosition = { 0.f, 0.f, 0.f };
	t.mLocalScale = { 1.3f, 1.3f, 1.3f };

	shared_ptr<Mesh> phereMesh;
	std::vector<shared_ptr<Material>> material2s;
	shared_ptr<Material> material2;
	vector<shared_ptr<Animator>> anmators;

	phereMesh = RESOURCEMANAGER.Get<Mesh>(L"SM_Hornman_Body");
	material2 = RESOURCEMANAGER.Get<Material>(L"Anim_Hornman_Run0");
	material2s.push_back(material2);
	anmators.push_back(RESOURCEMANAGER.Get<Animator>(L"Anim_Hornman_Run"));
	anmators.push_back(RESOURCEMANAGER.Get<Animator>(L"Anim_Hornman_Attack_01"));
	anmators.push_back(RESOURCEMANAGER.Get<Animator>(L"Anim_Hornman_Die"));

	switch (ctx.ViewAs<S2C_SpawnPacekt>()->Type) {
	case 0:
		world->AddComponent<HealthComponent>(mEntityID, 100, 100);
		world->AddComponent<EnemyComponent>(mEntityID, 0);
		break;
	}

	world->AddComponent<TransformComponent>(mEntityID, t);
	world->AddComponent<NetTransformComponent>(mEntityID);
	world->AddComponent<RenderComponent>(mEntityID, phereMesh, material2s);
	
	world->AddComponent<AnimationComponent>(mEntityID, anmators);
	
	Vec3 center{ 0,50,0 };
	Vec3 half{ 50,100,50 };
	world->AddComponent<BoxColliderComponent>(mEntityID,half, center);

	world->AddComponent<UIHpBarComponent>(mEntityID, 180.f, mEntityID, Vec3(0.f, 200.f, 0.f), 20.f, L"HPBAR_RUDWIG", L"HPBAR_IBANIX");

	auto& netComp = world->AddComponent<NetEntityComponent>(mEntityID);
	netComp.mOwnerEntity = mEntityID;
	netComp.mNetEntityId = ctx.ViewAs<S2C_SpawnPacekt>()->netEntityId;
	world->NetIdBinding(netComp.mNetEntityId, mEntityID);


	

	return mEntityID;
}

BulletPrefab::BulletPrefab(World* world)
{
}

BulletPrefab::~BulletPrefab()
{
}

Entity BulletPrefab::Build(World* world, const InputCommand& ctx)
{
	Entity mEntityID = world->CreateEntity();

	TransformComponent t{};
	t.mLocalPosition = { 0.f, 100.f, 0.f };
	t.mLocalScale = { 10.05f, 10.05f, 10.05f };

	world->AddComponent<TransformComponent>(mEntityID, t);
	//world->AddComponent<BoxColliderComponent>(mEntityID);

	auto& bulletComp = world->AddComponent<BulletComponent>(mEntityID);
	bulletComp.Activate(SkillType::Default, 0, 0, 0, t.mLocalPosition, Vec3::Forward, 90.0f, 2.0f, 10.0f);
	bulletComp.Deactivate();

	const S2C_SpawnPacekt* spawnPacket = ctx.ViewAs<S2C_SpawnPacekt>();
	if (spawnPacket == nullptr)
		return mEntityID;

	auto& netComp = world->AddComponent<NetEntityComponent>(mEntityID);
	netComp.mOwnerEntity = mEntityID;
	netComp.mNetEntityId = spawnPacket->netEntityId;
	world->NetIdBinding(netComp.mNetEntityId, mEntityID);

	return mEntityID;
}

HealPackPrefab::HealPackPrefab(World* world)
{
	(void)world;
}

HealPackPrefab::~HealPackPrefab()
{
}

Entity HealPackPrefab::Build(World* world, const InputCommand& ctx)
{

	return PrefabFactory::BuildWorldMarkerPrefab(world, ctx, L"VFX_Sector_Heal", Vec3(5.f, 5.f, 5.f));
}

JumpPadPrefab::JumpPadPrefab(World* world)
{
	(void)world;
}

JumpPadPrefab::~JumpPadPrefab()
{
}

Entity JumpPadPrefab::Build(World* world, const InputCommand& ctx)
{
	return PrefabFactory::BuildWorldMarkerPrefab(world, ctx, L"VFX_Sector_Jump", Vec3(15.f, 10.f, 15.f));
}

MonsterSpawnerMarkerPrefab::MonsterSpawnerMarkerPrefab(World* world)
{
	(void)world;
}

MonsterSpawnerMarkerPrefab::~MonsterSpawnerMarkerPrefab()
{
}

Entity MonsterSpawnerMarkerPrefab::Build(World* world, const InputCommand& ctx)
{
	return PrefabFactory::BuildWorldMarkerPrefab(world, ctx, L"VFX_Sector_Spawn", Vec3(5.f, 5.f, 5.f));
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
	bt.mLocalPosition = Vec3(-0.5 * 378 * 100, -41.6f, -0.5 * 378 * 100);


	world->AddComponent<TransformComponent>(mEntityID, bt);

	// heightmap 512x512 => 타일 511x511


	shared_ptr<Mesh> terrain = RESOURCEMANAGER.LoadTerrainMesh(378, 378);
	shared_ptr<Material> heightMap = RESOURCEMANAGER.Get<Material>(L"Terrain");

	TerrainComponent& terrainc = world->AddComponent<TerrainComponent>(mEntityID, 378, 378, heightMap);
	terrainc.mTerrainWorldPosition = bt.mLocalPosition;
	terrainc.mTerrainWorldScale = bt.mLocalScale;

	std::vector<shared_ptr<Material>> materials{
		
		RESOURCEMANAGER.Get<Material>(L"Grass"),
		RESOURCEMANAGER.Get<Material>(L"Sand_Rock"),
		RESOURCEMANAGER.Get<Material>(L"Dirt"),
		RESOURCEMANAGER.Get<Material>(L"Sand"),
		RESOURCEMANAGER.Get<Material>(L"Dirt_Road"),
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
	l.mLightInfo.Color.Ambient = { Vec3(0.5f, 0.5f, 0.5f) };
	l.mLightInfo.Color.Diffuse = { Vec3(1.0f, 1.0f, 1.0f) };
	l.mLightInfo.Color.Specular = { Vec3(0.1f, 0.1f, 0.1f) };
	l.SetLightDirection(Vec3(-0.0713f, -0.6448f, 0.7610f));
	// 방향벡터이므로 각도를 계산해서 넣어줘야할듯
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


HUDPortraitPrefab::HUDPortraitPrefab(World* world, uint8 playerType)
{
	const float  BounceAmplitude = 0.05f;
	const float  mBounceFrequency = 2.f;
	const float  mBounceDamping = 10.0f;


#ifdef _IMGUI

	std::vector<EditorProperty> props;
#endif
	{	// BACK 0
		Entity Portrait = world->CreateEntity();

		shared_ptr<Texture> scorem;
		switch (playerType) {
		case 0:
			scorem = RESOURCEMANAGER.Get<Texture>(L"UI_Rudwig_Portrait_0");
			break;
		case 1:
			scorem = RESOURCEMANAGER.Get<Texture>(L"UI_Ibanix_Portrait_0");
			break;
		case 2:
			scorem = RESOURCEMANAGER.Get<Texture>(L"UI_Fanthor_Portrait_0");
			break;
		}
		auto& t = world->AddComponent<UITransformComponent>(Portrait);
		t.mAnchor = Anchor::BottomLeft;
		t.mPosition = Vec2(64.f, -300.f);
		t.mSize = Vec2(256.f, 256.f);
		t.mUILayerIndex = 1;

		world->AddComponent<UISpriteComponent>(Portrait, scorem);
#ifdef _IMGUI



		props.push_back({ "Back Portrait0 Position",  PropertyType::Vec2,  &(t.mPosition),   0.f,    0.f });
		props.push_back({ "Back Portrait0 Size",  PropertyType::Vec2,  &(t.mSize),   0.f,    0.f });

#endif
	}
	{	// BACK 1
		Entity Portrait = world->CreateEntity();

		shared_ptr<Texture> scorem;
		switch (playerType) {
		case 0:
			scorem = RESOURCEMANAGER.Get<Texture>(L"UI_Rudwig_Portrait_1");
			break;
		case 1:
			scorem = RESOURCEMANAGER.Get<Texture>(L"UI_Ibanix_Portrait_1");
			break;
		case 2:
			scorem = RESOURCEMANAGER.Get<Texture>(L"UI_Fanthor_Portrait_1");
			break;
		}
		auto& t = world->AddComponent<UITransformComponent>(Portrait);
		t.mAnchor = Anchor::BottomLeft;
		t.mPosition = Vec2(64.f, -300.f);
		t.mSize = Vec2(256.f, 256.f);
		t.mUILayerIndex = 2;


		world->AddComponent<UISpriteComponent>(Portrait, scorem);
#ifdef _IMGUI



		

		props.push_back({ "Back Portrait1 Position",  PropertyType::Vec2,  &(t.mPosition),   0.f,    0.f });
		props.push_back({ "Back Portrait1 Size",  PropertyType::Vec2,  &(t.mSize),   0.f,    0.f });
		
		
#endif
	}
	{	// Portrait 0
		Entity Portrait = world->CreateEntity();

		shared_ptr<Texture> scorem;
		switch (playerType) {
		case 0:
			scorem = RESOURCEMANAGER.Get<Texture>(L"UI_Rudwig_Portrait_Head_0");
			break;
		case 1:
			scorem = RESOURCEMANAGER.Get<Texture>(L"UI_Ibanix_Portrait_Head_0");
			break;
		case 2:
			scorem = RESOURCEMANAGER.Get<Texture>(L"UI_Fanthor_Portrait_Head_0");
			break;
		}
		auto& t = world->AddComponent<UITransformComponent>(Portrait);
		t.mAnchor = Anchor::BottomLeft;
		t.mPosition = Vec2(64.f, -300.f);
		t.mSize = Vec2(256.f, 256.f);
		t.mUILayerIndex = 3;

		world->AddComponent<UISpriteComponent>(Portrait, scorem);


#ifdef _IMGUI



		
		props.push_back({ "Portrait0 Position",  PropertyType::Vec2,  &(t.mPosition),  0.f,    0.f });
		props.push_back({ "Portrait0 Size",  PropertyType::Vec2,  &(t.mSize),   0.f,    0.f });
		
	
#endif

	}
	{	// Portrait 1
		Entity Portrait1 = world->CreateEntity();

		shared_ptr<Texture> scorem;
		switch (playerType) {
		case 0:
			scorem = RESOURCEMANAGER.Get<Texture>(L"UI_Rudwig_Portrait_Head_1");
			break;
		case 1:
			scorem = RESOURCEMANAGER.Get<Texture>(L"UI_Ibanix_Portrait_Head_1");
			break;
		case 2:
			scorem = RESOURCEMANAGER.Get<Texture>(L"UI_Fanthor_Portrait_Head_1");
			break;
		}
		auto& t = world->AddComponent<UITransformComponent>(Portrait1);
		t.mAnchor = Anchor::BottomLeft;
		t.mPosition = Vec2(64.f, -300.f);
		t.mSize = Vec2(256.f, 256.f);
		t.mUILayerIndex = 4;

		auto& m = world->AddComponent<UIActionComponent>(Portrait1);
		m.mDuration = 0.5f;
		m.mActor = UIActor::Player;
		m.mState = UIActionState::Bounce;
		m.mIsLoop = true;
		m.mBounceAmplitude = BounceAmplitude;
		m.mBounceFrequency = mBounceFrequency;
		m.mBounceDamping = mBounceDamping;
		world->AddComponent<UISpriteComponent>(Portrait1, scorem);
#ifdef _IMGUI



		IMGUIComponent& visImgui = world->AddComponent<IMGUIComponent>(Portrait1);
		props.push_back({ "Portrait1 Position",  PropertyType::Vec2,  &(t.mPosition),  0.f,    0.f });
		props.push_back({ "Portrait1 Size",  PropertyType::Vec2,  &(t.mSize),  0.f,    0.f });
		visImgui.RegisterEditorProperties(props);
		visImgui.SetName("Menu4");
#endif
	}





#ifdef _IMGUI

	std::vector<EditorProperty> props2;
#endif
	{	// BACK 0
		Entity Portrait = world->CreateEntity();

		shared_ptr<Texture> scorem;
		switch (0) {
		case 0:
			scorem = RESOURCEMANAGER.Get<Texture>(L"UI_Rudwig_Portrait_0");
			break;
		case 1:
			scorem = RESOURCEMANAGER.Get<Texture>(L"UI_Ibanix_Portrait_0");
			break;
		case 2:
			scorem = RESOURCEMANAGER.Get<Texture>(L"UI_Fanthor_Portrait_0");
			break;
		}
		auto& t = world->AddComponent<UITransformComponent>(Portrait);
		t.mAnchor = Anchor::BottomLeft;
		t.mPosition = Vec2(64.f, -464.f);
		t.mSize = Vec2(160, 160);
		t.mUILayerIndex = 1;

		world->AddComponent<UISpriteComponent>(Portrait, scorem);
#ifdef _IMGUI



		props2.push_back({ "Back Portrait0 Position",  PropertyType::Vec2,  &(t.mPosition),   0.f,    0.f });
		props2.push_back({ "Back Portrait0 Size",  PropertyType::Vec2,  &(t.mSize),   0.f,    0.f });

#endif
	}
	{	// BACK 1
		Entity Portrait = world->CreateEntity();

		shared_ptr<Texture> scorem;
		switch (0) {
		case 0:
			scorem = RESOURCEMANAGER.Get<Texture>(L"UI_Rudwig_Portrait_1");
			break;
		case 1:
			scorem = RESOURCEMANAGER.Get<Texture>(L"UI_Ibanix_Portrait_1");
			break;
		case 2:
			scorem = RESOURCEMANAGER.Get<Texture>(L"UI_Fanthor_Portrait_1");
			break;
		}
		auto& t = world->AddComponent<UITransformComponent>(Portrait);
		t.mAnchor = Anchor::BottomLeft;
		t.mPosition = Vec2(64.f, -464.f);
		t.mSize = Vec2(160, 160);
		t.mUILayerIndex = 2;


		world->AddComponent<UISpriteComponent>(Portrait, scorem);
#ifdef _IMGUI





		props2.push_back({ "Back Portrait1 Position",  PropertyType::Vec2,  &(t.mPosition),   0.f,    0.f });
		props2.push_back({ "Back Portrait1 Size",  PropertyType::Vec2,  &(t.mSize),   0.f,    0.f });


#endif
	}
	{	// Portrait 0
		Entity Portrait = world->CreateEntity();

		shared_ptr<Texture> scorem;
		switch (0) {
		case 0:
			scorem = RESOURCEMANAGER.Get<Texture>(L"UI_Rudwig_Portrait_Head_0");
			break;
		case 1:
			scorem = RESOURCEMANAGER.Get<Texture>(L"UI_Ibanix_Portrait_Head_0");
			break;
		case 2:
			scorem = RESOURCEMANAGER.Get<Texture>(L"UI_Fanthor_Portrait_Head_0");
			break;
		}
		auto& t = world->AddComponent<UITransformComponent>(Portrait);
		t.mAnchor = Anchor::BottomLeft;
		t.mPosition = Vec2(64.f, -464.f);
		t.mSize = Vec2(160, 160);
		t.mUILayerIndex = 3;

		world->AddComponent<UISpriteComponent>(Portrait, scorem);


#ifdef _IMGUI




		props2.push_back({ "Portrait0 Position",  PropertyType::Vec2,  &(t.mPosition),  0.f,    0.f });
		props2.push_back({ "Portrait0 Size",  PropertyType::Vec2,  &(t.mSize),   0.f,    0.f });


#endif

	}
	{	// Portrait 1
		Entity Portrait1 = world->CreateEntity();

		shared_ptr<Texture> scorem;
		switch (0) {
		case 0:
			scorem = RESOURCEMANAGER.Get<Texture>(L"UI_Rudwig_Portrait_Head_1");
			break;
		case 1:
			scorem = RESOURCEMANAGER.Get<Texture>(L"UI_Ibanix_Portrait_Head_1");
			break;
		case 2:
			scorem = RESOURCEMANAGER.Get<Texture>(L"UI_Fanthor_Portrait_Head_1");
			break;
		}
		auto& t = world->AddComponent<UITransformComponent>(Portrait1);
		t.mAnchor = Anchor::BottomLeft;
		t.mPosition = Vec2(64.f, -464.f);
		t.mSize = Vec2(160, 160);
		t.mUILayerIndex = 4;

		auto& m = world->AddComponent<UIActionComponent>(Portrait1);
		m.mDuration = 0.5f;
		m.mActor = UIActor::Player;
		m.mState = UIActionState::Bounce;
		m.mIsLoop = true;
		m.mBounceAmplitude = BounceAmplitude;
		m.mBounceFrequency = mBounceFrequency;
		m.mBounceDamping = mBounceDamping;
		world->AddComponent<UISpriteComponent>(Portrait1, scorem);
#ifdef _IMGUI



		IMGUIComponent& visImgui = world->AddComponent<IMGUIComponent>(Portrait1);
		props2.push_back({ "Portrait1 Position",  PropertyType::Vec2,  &(t.mPosition),  0.f,    0.f });
		props2.push_back({ "Portrait1 Size",  PropertyType::Vec2,  &(t.mSize),  0.f,    0.f });
		visImgui.RegisterEditorProperties(props2);
		visImgui.SetName("Menu5");
#endif
	}


#ifdef _IMGUI

	std::vector<EditorProperty> props3;
#endif
	{	// BACK 0
		Entity Portrait = world->CreateEntity();

		shared_ptr<Texture> scorem;
		switch (1) {
		case 0:
			scorem = RESOURCEMANAGER.Get<Texture>(L"UI_Rudwig_Portrait_0");
			break;
		case 1:
			scorem = RESOURCEMANAGER.Get<Texture>(L"UI_Ibanix_Portrait_0");
			break;
		case 2:
			scorem = RESOURCEMANAGER.Get<Texture>(L"UI_Fanthor_Portrait_0");
			break;
		}
		auto& t = world->AddComponent<UITransformComponent>(Portrait);
		t.mAnchor = Anchor::BottomLeft;
		t.mPosition = Vec2(64.f, -640.f);
		t.mSize = Vec2(160, 160);
		t.mUILayerIndex = 1;

		world->AddComponent<UISpriteComponent>(Portrait, scorem);
#ifdef _IMGUI



		props3.push_back({ "Back Portrait0 Position",  PropertyType::Vec2,  &(t.mPosition),   0.f,    0.f });
		props3.push_back({ "Back Portrait0 Size",  PropertyType::Vec2,  &(t.mSize),   0.f,    0.f });

#endif
	}
	{	// BACK 1
		Entity Portrait = world->CreateEntity();

		shared_ptr<Texture> scorem;
		switch (1) {
		case 0:
			scorem = RESOURCEMANAGER.Get<Texture>(L"UI_Rudwig_Portrait_1");
			break;
		case 1:
			scorem = RESOURCEMANAGER.Get<Texture>(L"UI_Ibanix_Portrait_1");
			break;
		case 2:
			scorem = RESOURCEMANAGER.Get<Texture>(L"UI_Fanthor_Portrait_1");
			break;
		}
		auto& t = world->AddComponent<UITransformComponent>(Portrait);
		t.mAnchor = Anchor::BottomLeft;
		t.mPosition = Vec2(64.f, -640.f);
		t.mSize = Vec2(160, 160);
		t.mUILayerIndex = 2;


		world->AddComponent<UISpriteComponent>(Portrait, scorem);
#ifdef _IMGUI





		props3.push_back({ "Back Portrait1 Position",  PropertyType::Vec2,  &(t.mPosition),   0.f,    0.f });
		props3.push_back({ "Back Portrait1 Size",  PropertyType::Vec2,  &(t.mSize),   0.f,    0.f });


#endif
	}
	{	// Portrait 0
		Entity Portrait = world->CreateEntity();

		shared_ptr<Texture> scorem;
		switch (1) {
		case 0:
			scorem = RESOURCEMANAGER.Get<Texture>(L"UI_Rudwig_Portrait_Head_0");
			break;
		case 1:
			scorem = RESOURCEMANAGER.Get<Texture>(L"UI_Ibanix_Portrait_Head_0");
			break;
		case 2:
			scorem = RESOURCEMANAGER.Get<Texture>(L"UI_Fanthor_Portrait_Head_0");
			break;
		}
		auto& t = world->AddComponent<UITransformComponent>(Portrait);
		t.mAnchor = Anchor::BottomLeft;
		t.mPosition = Vec2(64.f, -640.f);
		t.mSize = Vec2(160, 160);
		t.mUILayerIndex = 3;

		world->AddComponent<UISpriteComponent>(Portrait, scorem);


#ifdef _IMGUI




		props3.push_back({ "Portrait0 Position",  PropertyType::Vec2,  &(t.mPosition),  0.f,    0.f });
		props3.push_back({ "Portrait0 Size",  PropertyType::Vec2,  &(t.mSize),   0.f,    0.f });


#endif

	}
	{	// Portrait 1
		Entity Portrait1 = world->CreateEntity();

		shared_ptr<Texture> scorem;
		switch (1) {
		case 0:
			scorem = RESOURCEMANAGER.Get<Texture>(L"UI_Rudwig_Portrait_Head_1");
			break;
		case 1:
			scorem = RESOURCEMANAGER.Get<Texture>(L"UI_Ibanix_Portrait_Head_1");
			break;
		case 2:
			scorem = RESOURCEMANAGER.Get<Texture>(L"UI_Fanthor_Portrait_Head_1");
			break;
		}
		auto& t = world->AddComponent<UITransformComponent>(Portrait1);
		t.mAnchor = Anchor::BottomLeft;
		t.mPosition = Vec2(64.f, -640.f);
		t.mSize = Vec2(160, 160);
		t.mUILayerIndex = 4;

		auto& m = world->AddComponent<UIActionComponent>(Portrait1);
		m.mDuration = 0.5f;
		m.mActor = UIActor::Player;
		m.mState = UIActionState::Bounce;
		m.mIsLoop = true;
		m.mBounceAmplitude = BounceAmplitude;
		m.mBounceFrequency = mBounceFrequency;
		m.mBounceDamping = mBounceDamping;
		world->AddComponent<UISpriteComponent>(Portrait1, scorem);
#ifdef _IMGUI



		IMGUIComponent& visImgui = world->AddComponent<IMGUIComponent>(Portrait1);
		props3.push_back({ "Portrait1 Position",  PropertyType::Vec2,  &(t.mPosition),  0.f,    0.f });
		props3.push_back({ "Portrait1 Size",  PropertyType::Vec2,  &(t.mSize),  0.f,    0.f });
		visImgui.RegisterEditorProperties(props2);
		visImgui.SetName("Menu6");
#endif
	}







}

HUDPortraitPrefab::~HUDPortraitPrefab()
{
}

HUDHPBarPrefab::HUDHPBarPrefab(World* world, uint8 playerType, Entity ownerEntity)
{
	{

#ifdef _IMGUI

		std::vector<EditorProperty> props;
#endif
		{	// BACK 0
			Entity back = world->CreateEntity();

			shared_ptr<Texture> scorem;
			switch (playerType) {
			case 0:
				scorem = RESOURCEMANAGER.Get<Texture>(L"UI_Rudwig_HP_0");
				break;
			case 1:
				scorem = RESOURCEMANAGER.Get<Texture>(L"UI_Ibanix_HP_0");
				break;
			case 2:
				scorem = RESOURCEMANAGER.Get<Texture>(L"UI_Fanthor_HP_0");
				break;
			}
			auto& t = world->AddComponent<UITransformComponent>(back);
			t.mAnchor = Anchor::Center;
			t.mPosition = Vec2(0.f, 576.f);
			t.mSize = Vec2(768.f, 96.f);
			t.mPivot = Vec2(0.5f, 0.5f);
			t.mUILayerIndex = 1;

			world->AddComponent<UISpriteComponent>(back, scorem);
#ifdef _IMGUI



			props.push_back({ "Background HP Back Pos",  PropertyType::Vec2,  &(t.mPosition),   0.f,    0.f });
			props.push_back({ "Background HP Back Size",  PropertyType::Vec2,  &(t.mSize),   0.f,    0.f });

#endif
		}
		{	// BACK 1
			Entity hpback = world->CreateEntity();

			shared_ptr<Texture> scorem;
			switch (playerType) {
			case 0:
				scorem = RESOURCEMANAGER.Get<Texture>(L"UI_Player_HP_1");
				break;
			case 1:
				scorem = RESOURCEMANAGER.Get<Texture>(L"UI_Player_HP_1");
				break;
			case 2:
				scorem = RESOURCEMANAGER.Get<Texture>(L"UI_Player_HP_1");
				break;
			}
			auto& t = world->AddComponent<UITransformComponent>(hpback);
			t.mAnchor = Anchor::Center;
			t.mPosition = Vec2(-256.f, 548.f);
			t.mSize = Vec2(512.f, 96.f);
			t.mPivot = Vec2(0.0f, 0.0f);
			t.mUILayerIndex = 3;


			world->AddComponent<UISpriteComponent>(hpback, scorem);
#ifdef _IMGUI

			props.push_back({ "Back HP Back Pos",  PropertyType::Vec2,  &(t.mPosition),   0.f,    0.f });
			props.push_back({ "Back HP Back Size",  PropertyType::Vec2,  &(t.mSize),   0.f,    0.f });

#endif
		}
		{

			Entity hp = world->CreateEntity();

			shared_ptr<Texture> scorem;
			switch (playerType) {
			case 0:
				scorem = RESOURCEMANAGER.Get<Texture>(L"UI_Player_HP_2");
				break;
			case 1:
				scorem = RESOURCEMANAGER.Get<Texture>(L"UI_Player_HP_2");
				break;
			case 2:
				scorem = RESOURCEMANAGER.Get<Texture>(L"UI_Player_HP_2");
				break;
			}
			auto& t = world->AddComponent<UITransformComponent>(hp);
			t.mAnchor = Anchor::Center;
			t.mPosition = Vec2(-256.f, 548.f);
			t.mSize = Vec2(512.f, 96.f);
			t.mPivot = Vec2(0.0f, 0.0f);
			t.mUILayerIndex = 2;


			world->AddComponent<UISpriteComponent>(hp, scorem);
			world->AddComponent<UIScriptComponent>(hp).mOnUpdate = (hp, [world, hp, ownerEntity](float dt) {


				world->GetEventManager()->Consume<EvHealthChanged>([world, hp, ownerEntity](const EvHealthChanged& e) {
					if (e.target != ownerEntity) return;
					// 피 딱 잘리게 0~1 사이로 정규화해서 넣어주면 될듯
					// 그러기 위해 pivot이 0,0이 되어야할듯
					UISpriteComponent* s = world->GetComponent<UISpriteComponent>(hp);
					s->SetVisibleRangeKeepDestinationSize(false);


					s->SetVisibleRangeNormalized(0.f,( (float) e.hp / e.maxHp));
					std::cout << "Health Changed: " << e.hp << "/" << e.maxHp << std::endl;

					});
			});
#ifdef _IMGUI

			props.push_back({ "Back Hp Position",  PropertyType::Vec2,  &(t.mPosition),   0.f,    0.f });
			props.push_back({ "Back Hp Size",  PropertyType::Vec2,  &(t.mSize),   0.f,    0.f });

#endif

		}
		{

			Entity hp = world->CreateEntity();
			auto& t = world->AddComponent<UITransformComponent>(hp);
			t.mAnchor = Anchor::Center;
			t.mPosition = Vec2(-256.f, 548.f);
			t.mSize = Vec2(512.f, 96.f);
			t.mPivot = Vec2(0.0f, 0.0f);
			t.mUILayerIndex = 2;

			auto& text = world->AddComponent<UITextComponent>(hp);
			text.mText = L"HP";
				text.mOnTextChanged = [world, hp, ownerEntity]() {
					world->GetEventManager()->Consume<EvHealthChanged>([world, hp, ownerEntity](const EvHealthChanged& e) {
						if (e.target != ownerEntity) return;
						UITextComponent* t = world->GetComponent<UITextComponent>(hp);
						if (t) t->mText = std::to_wstring(e.hp) + L"/" + std::to_wstring(e.maxHp);
						});
					};

			
#ifdef _IMGUI

			props.push_back({ "Font Hp Position",  PropertyType::Vec2,  &(t.mPosition),   0.f,    0.f });
			props.push_back({ "Font Hp Size",  PropertyType::Vec2,  &(t.mSize),   0.f,    0.f });

#endif

		



#ifdef _IMGUI


			IMGUIComponent& visImgui = world->AddComponent<IMGUIComponent>(hp);
			visImgui.RegisterEditorProperties(props);
			visImgui.SetName("HP");
#endif
		}




	}
}

HUDHPBarPrefab::~HUDHPBarPrefab()
{

}

HUDWeaponPrefab::HUDWeaponPrefab(World* world, uint8 playerType, Entity ownerEntity)
{

	{
		const float  BounceAmplitude = 0.05f;
		const float  mBounceFrequency = 2.f;
		const float  mBounceDamping = 10.0f;


#ifdef _IMGUI

		std::vector<EditorProperty> props;
#endif
		{	// BACK 0
			Entity weapon = world->CreateEntity();

			shared_ptr<Texture> scorem;
			switch (playerType) {
			case 0:
				scorem = RESOURCEMANAGER.Get<Texture>(L"UI_Rudwig_Display_0");
				break;
			case 1:
				scorem = RESOURCEMANAGER.Get<Texture>(L"UI_Ibanix_Display_01");
				break;
			case 2:
				scorem = RESOURCEMANAGER.Get<Texture>(L"UI_Fanthor_Display_0");
				break;
			}
			auto& t = world->AddComponent<UITransformComponent>(weapon);
			t.mAnchor = Anchor::BottomLeft;
			t.mPosition = Vec2(352.f, -160.f);
			t.mSize = Vec2(128.f, 96.f);
			t.mUILayerIndex = 1;
			t.mPivot = Vec2(0.5f, 0.5f);
			world->AddComponent<UISpriteComponent>(weapon, scorem);
#ifdef _IMGUI



			props.push_back({ "Weaponback pos",  PropertyType::Vec2,  &(t.mPosition),   0.f,    0.f });
			props.push_back({ "Weaponback size",  PropertyType::Vec2,  &(t.mSize),   0.f,    0.f });

#endif
		}
		{	// Bullet
			Entity bullet = world->CreateEntity();

			UITextComponent& text = world->AddComponent<UITextComponent>(bullet);
			text.mText = L"EMPTY";
			switch (playerType) {
			case 0:
			{
				text.mText = L"OO";
				
			}
				break;
			case 1:
			{


				text.mText = L"10/10";
				text.mOnTextChanged = [world, bullet]() {
					world->GetEventManager()->Consume<EvBulletCountChanged>([world, bullet](const EvBulletCountChanged& e) {
						if (e.target != bullet) return;
						UITextComponent* t = world->GetComponent<UITextComponent>(bullet);
						if (t) t->mText = std::to_wstring(e.current) + L"/" + std::to_wstring(e.max);
						});
					};
			}
				break;
			case 2:
			{
				text.mText = L"5/5";
				text.mOnTextChanged = [world, bullet]() {
					world->GetEventManager()->Consume<EvBulletCountChanged>([world, bullet](const EvBulletCountChanged& e) {
						if (e.target != bullet) return;
						UITextComponent* t = world->GetComponent<UITextComponent>(bullet);
						if (t) t->mText = std::to_wstring(e.current) + L"/" + std::to_wstring(e.max);
						});
					};
			}
				break;
			}
			auto& t = world->AddComponent<UITransformComponent>(bullet);
			t.mAnchor = Anchor::BottomLeft;
			t.mPosition = Vec2(576.f, -160.f);
			t.mSize = Vec2(128.f, 96.f);
			t.mUILayerIndex = 1;
			t.mPivot = Vec2(0.5f, 0.5f);
			//world->AddComponent<UISpriteComponent>(bullet, scorem);
			
			
#ifdef _IMGUI



			props.push_back({ "Bullet pos",  PropertyType::Vec2,  &(t.mPosition),   0.f,    0.f });
			props.push_back({ "Bullet size",  PropertyType::Vec2,  &(t.mSize),   0.f,    0.f });

#endif
		}
		{	// Weapon
			Entity sound = world->CreateEntity();

			shared_ptr<Texture> scorem;
			switch (playerType) {
			case 0:
				scorem = RESOURCEMANAGER.Get<Texture>(L"UI_Rudwig_Weapon_0");
				break;
			case 1:
				scorem = RESOURCEMANAGER.Get<Texture>(L"UI_Ibanix_Weapon_0");
				break;
			case 2:
				scorem = RESOURCEMANAGER.Get<Texture>(L"UI_Fanthor_Weapon_0");
				break;
			}
			auto& t = world->AddComponent<UITransformComponent>(sound);
			t.mAnchor = Anchor::BottomLeft;
			t.mPosition = Vec2(432.f, -160.f);
			t.mSize = Vec2(128.f, 64.f);
			t.mUILayerIndex = 2;
			t.mPivot = Vec2(0.5f, 0.5f);

			world->AddComponent<UISpriteComponent>(sound, scorem);
#ifdef _IMGUI

			props.push_back({ "Weapon pos",  PropertyType::Vec2,  &(t.mPosition),   0.f,    0.f });
			props.push_back({ "Weapon size",  PropertyType::Vec2,  &(t.mSize),   0.f,    0.f });

#endif




			
#ifdef _IMGUI
		

			IMGUIComponent& visImgui = world->AddComponent<IMGUIComponent>(sound);
			visImgui.RegisterEditorProperties(props);
			visImgui.SetName("Weapon");
#endif
		}




	}
}

HUDWeaponPrefab::~HUDWeaponPrefab()
{
}



HUDMusicPrefab::HUDMusicPrefab(World* world, uint8 playerType, Entity ownerEntity)
{
	{
		const float  BounceAmplitude = 0.05f;
		const float  mBounceFrequency = 2.f;
		const float  mBounceDamping = 10.0f;


#ifdef _IMGUI

		std::vector<EditorProperty> props;
#endif
		{	// BACK 0
			
			Entity back = world->CreateEntity();

			shared_ptr<Texture> scorem;
			switch (playerType) {
			case 0:
				scorem = RESOURCEMANAGER.Get<Texture>(L"UI_Rudwig_Display_0");
				break;
			case 1:
				scorem = RESOURCEMANAGER.Get<Texture>(L"UI_Ibanix_Display_01");
				break;
			case 2:
				scorem = RESOURCEMANAGER.Get<Texture>(L"UI_Fanthor_Display_0");
				break;
			}
			auto& t = world->AddComponent<UITransformComponent>(back);
			t.mAnchor = Anchor::BottomLeft;
			t.mPosition = Vec2(352.f, -96.f);
			t.mSize = Vec2(128.f, 96.f);
			t.mUILayerIndex = 2;
			t.mPivot = Vec2(0.5f, 0.5f);

			world->AddComponent<UISpriteComponent>(back, scorem);
#ifdef _IMGUI



			props.push_back({ "MusicBack pos ",  PropertyType::Vec2,  &(t.mPosition),   0.f,    0.f });
			props.push_back({ "MusicBack size",  PropertyType::Vec2,  &(t.mSize),   0.f,    0.f });

#endif
		}
		{	// BACK 1
			Entity sound = world->CreateEntity();

			std::vector<shared_ptr<Texture>> scorem;
			switch (playerType) {
			case 0:
				scorem.push_back(RESOURCEMANAGER.Get<Texture>(L"UI_Rudwig_Rhythm_Text_0"));
				scorem.push_back(RESOURCEMANAGER.Get<Texture>(L"UI_Rudwig_Rhythm_Text_1"));
				scorem.push_back(RESOURCEMANAGER.Get<Texture>(L"UI_Rudwig_Rhythm_Text_2"));
				scorem.push_back(RESOURCEMANAGER.Get<Texture>(L"UI_Rudwig_Rhythm_Text_3"));
				break;
			case 1:
				scorem.push_back(RESOURCEMANAGER.Get<Texture>(L"UI_Ibanix_Rhythm_Text_0"));
				scorem.push_back(RESOURCEMANAGER.Get<Texture>(L"UI_Ibanix_Rhythm_Text_1"));
				scorem.push_back(RESOURCEMANAGER.Get<Texture>(L"UI_Ibanix_Rhythm_Text_2"));
				scorem.push_back(RESOURCEMANAGER.Get<Texture>(L"UI_Ibanix_Rhythm_Text_3"));
				break;
			case 2:
				scorem.push_back(RESOURCEMANAGER.Get<Texture>(L"UI_Fanthor_Rhythm_Text_0"));
				scorem.push_back(RESOURCEMANAGER.Get<Texture>(L"UI_Fanthor_Rhythm_Text_1"));
				scorem.push_back(RESOURCEMANAGER.Get<Texture>(L"UI_Fanthor_Rhythm_Text_2"));
				scorem.push_back(RESOURCEMANAGER.Get<Texture>(L"UI_Fanthor_Rhythm_Text_3"));
				break;
			}
			auto& t = world->AddComponent<UITransformComponent>(sound);
			t.mAnchor = Anchor::BottomLeft;
			t.mPosition = Vec2(480.f, -88.f);
			t.mSize = Vec2(256.f, 96.f);
			t.mUILayerIndex = 1;
			t.mPivot = Vec2(0.5f, 0.5f);
			world->AddComponent<UISpriteComponent>(sound, scorem);
			UIScriptComponent& script = world->AddComponent<UIScriptComponent>(sound);
			script.mOnUpdate = [world, sound, scorem](float deltaTime) {
				
				world->GetEventManager()->Consume<EvRhythmChanged>([world, sound, scorem](const EvRhythmChanged& e) {
					// 비트 이벤트가 발생할 때마다 텍스처 변경
					UISpriteComponent* sprite = world->GetComponent<UISpriteComponent>(sound);
					if (sprite) {
						sprite->mTexture = scorem[e.musicNum]; // 리듬 단계에 따라 텍스처 변경
					}
					});

				
				};
			



#ifdef _IMGUI

			props.push_back({ "MusicName pos",  PropertyType::Vec2,  &(t.mPosition),   0.f,    0.f });
			props.push_back({ "MusicName size",  PropertyType::Vec2,  &(t.mSize),   0.f,    0.f });

#endif





#ifdef _IMGUI


			IMGUIComponent& visImgui = world->AddComponent<IMGUIComponent>(sound);
			visImgui.RegisterEditorProperties(props);
			visImgui.SetName("Music");
#endif
		}




	}

	//{	// BACK 0

	//	Entity Bot = world->CreateEntity();

	//	shared_ptr<Texture> scorem = RESOURCEMANAGER.Get<Texture>(L"UI_Ingame_Back");
	//
	//	auto& t = world->AddComponent<UITransformComponent>(Bot);
	//	t.mAnchor = Anchor::Center;
	//	t.mSize = Vec2(2560.f, 1440.f);
	//	t.mUILayerIndex = 2;
	//	t.mPivot = Vec2(0.5f, 0.5f);

	//	world->AddComponent<UISpriteComponent>(Bot, scorem);
	//}

}

HUDMusicPrefab::~HUDMusicPrefab()
{

}
