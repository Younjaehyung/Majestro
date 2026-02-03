#include "pch.h"
#include "Scene.h"
#include "Engine.h"
#include "EnginePch.h"
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
#include "AnimationComponent.h"
#include "TerrainComponent.h"
#include "UITransformComponent.h"
#include "UISpriteComponent.h"
#include "BeatComponent.h"
#include "GravityComponent.h"
#include "MovementComponent.h"
#include "VfxComponent.h"
#include "Prefab.h"

//#include "Camera.h"
//
//#include "ConstantBuffer.h"
//#include "Light.h"
//#include "Resources.h"


void Scene::Initialize()
{
	
}

void Scene::Update(float deltaTime)
{
	//mWorld->Update(deltaTime);
}

void Scene::Render()
{
	//mWorld->Render();
}

void LobbyScene::Initialize()
{
	//PlayerPrefab player{mWorld.get()};
	mWorld->SetSceneId(SceneId::Lobby);
	PrefabFactory::RegisterAllPrefabs();
	TerrainPrefab terrain{ mWorld.get() };
	SkyBoxPrefab skybox{ mWorld.get() };
	DirLightPrefab light{ mWorld.get() };
	//EnemyPrefab	enemys {mWorld.get() };

	/////////////////////////////////////////////////////////////////////

	{
		Entity mannequinEntity = mWorld->CreateEntity();
		mWorld->AddComponent<ChoicePlayerComponent>(mannequinEntity, 1);
		//mWorld->AddComponent<PlayerMovementComponent>(mannequinEntity);
		
	}

	/////////////////////////////////////////////////////////////////////
	{
		Entity vfxEntity = mWorld->CreateEntity();
		TransformComponent vfxTransform{};
		vfxTransform.mLocalPosition = Vec3(0.f, 35.f, 0.f);
		shared_ptr<Vfx> vfx = RESOURCEMANAGER.Get<Vfx>(L"vfx_dissolve_NoteBoar");
		mWorld->AddComponent<TransformComponent>(vfxEntity, vfxTransform);
		VfxComponent& vfxComp = mWorld->AddComponent<VfxComponent>(vfxEntity);
		vfxComp.mVfx = vfx;
	}
	/////////////////////////////////////////////////////////////////////


#pragma region UI
	{
		Entity hpBAR = mWorld->CreateEntity();
		shared_ptr<Material> scorem;
		scorem = RESOURCEMANAGER.Get<Material>(L"HPBAR");

		auto& t = mWorld->AddComponent<UITransformComponent>(hpBAR);
		t.mAnchor = Anchor::Center;
		t.mPosition = Vec2(-256.f, 768.f);
		t.mSize = Vec2(512.f, 256.f);


		auto& m = mWorld->AddComponent<UISpriteComponent>(hpBAR, scorem);
	}

	{
		Entity Aim = mWorld->CreateEntity();
		shared_ptr<Material> scorem;
		scorem = RESOURCEMANAGER.Get<Material>(L"Aim");

		auto& t = mWorld->AddComponent<UITransformComponent>(Aim);
		t.mAnchor = Anchor::Center;
		t.mPosition = Vec2(-64.f, -64.f);
		t.mSize = Vec2(128.f, 128.f);

		auto& m = mWorld->AddComponent<UISpriteComponent>(Aim, scorem);
	}

	{
		Entity Ibanix_Ammo = mWorld->CreateEntity();
		shared_ptr<Material> scorem;
		scorem = RESOURCEMANAGER.Get<Material>(L"Ibanix_Ammo");

		auto& t = mWorld->AddComponent<UITransformComponent>(Ibanix_Ammo);
		t.mAnchor = Anchor::BottomLeft;
		t.mPosition = Vec2(212.f, -212.f);
		t.mSize = Vec2(196.f, 128.f);

		auto& m = mWorld->AddComponent<UISpriteComponent>(Ibanix_Ammo, scorem);
	}

	{
		Entity Fanthor_Portrait = mWorld->CreateEntity();

		shared_ptr<Material> scorem;
		scorem = RESOURCEMANAGER.Get<Material>(L"Fanthor_Portrait");

		auto& t = mWorld->AddComponent<UITransformComponent>(Fanthor_Portrait);
		t.mAnchor = Anchor::BottomLeft;
		t.mPosition = Vec2(32.f, -636.f);
		t.mSize = Vec2(196.f, 196.f);

		auto& m = mWorld->AddComponent<UISpriteComponent>(Fanthor_Portrait, scorem);
	}

	{
		Entity Ibanix_Portrait = mWorld->CreateEntity();

		shared_ptr<Material> scorem;
		scorem = RESOURCEMANAGER.Get<Material>(L"Ibanix_Portrait");

		auto& t = mWorld->AddComponent<UITransformComponent>(Ibanix_Portrait);
		t.mAnchor = Anchor::BottomLeft;
		t.mPosition = Vec2(32.f, -424.f);
		t.mSize = Vec2(196.f, 196.f);

		auto& m = mWorld->AddComponent<UISpriteComponent>(Ibanix_Portrait, scorem);
	}

	{
		Entity Rudwig_Portrait = mWorld->CreateEntity();

		shared_ptr<Material> scorem;
		scorem = RESOURCEMANAGER.Get<Material>(L"Rudwig_Portrait");

		auto& t = mWorld->AddComponent<UITransformComponent>(Rudwig_Portrait);
		t.mAnchor = Anchor::BottomLeft;
		t.mPosition = Vec2(32.f, -212.f);
		t.mSize = Vec2(196.f, 196.f);

		auto& m = mWorld->AddComponent<UISpriteComponent>(Rudwig_Portrait, scorem);
	}

#pragma endregion

	/////////////////////////////////////////////////////////////////////////



	mWorld->Initialize();
}

void LobbyScene::Update(float deltaTime)
{
	mWorld->Update(deltaTime);
}

void LobbyScene::Render()
{
	mWorld->Render();
}

/// //////////////////////////////////////////////////////////////////////////////////
void GameScene::Initialize()
{
	mWorld->SetSceneId(SceneId::Game);
	//PlayerPrefab player{mWorld.get()};
	PrefabFactory::RegisterAllPrefabs();
	TerrainPrefab terrain{ mWorld.get() };
	SkyBoxPrefab skybox{ mWorld.get() };
	DirLightPrefab light{ mWorld.get() };
	//EnemyPrefab	enemys {mWorld.get() };



	/////////////////////////////////////////////////////////////////////
	{
		Entity vfxEntity = mWorld->CreateEntity();
		TransformComponent vfxTransform{};
		vfxTransform.mLocalPosition = Vec3(0.f, 35.f, 0.f);
		shared_ptr<Vfx> vfx = RESOURCEMANAGER.Get<Vfx>(L"vfx_dissolve_NoteBoar");
		mWorld->AddComponent<TransformComponent>(vfxEntity, vfxTransform);
		VfxComponent& vfxComp = mWorld->AddComponent<VfxComponent>(vfxEntity);
		vfxComp.mVfx = vfx;
	}
	/////////////////////////////////////////////////////////////////////

	{
		Entity osw = mWorld->CreateEntity();	// �ʼ�

		shared_ptr<Mesh> phereMesh = RESOURCEMANAGER.Get<Mesh>(L"Ammor");
		std::vector<shared_ptr<Material>> material2s;


		shared_ptr<Material> material2 = RESOURCEMANAGER.Get<Material>(L"oo10");
		material2s.push_back(material2);
		TransformComponent t{};
		t.mLocalPosition = { 0.f, 0.f, 0.f };
		t.mLocalScale = { 1.f, 1.f, 1.f };
		vector<shared_ptr<Animator>> anmators;
		anmators.push_back(RESOURCEMANAGER.Get<Animator>(L"Model|Punch"));


		mWorld->AddComponent<TransformComponent>(osw, t);
		mWorld->AddComponent<RenderComponent>(osw, phereMesh, material2s);
		//mWorld->AddComponent<AnimationComponent>(osw, anmators);
		float i, j, k;
		float n = 10;
		for (i = -50; i < 50; i += 10.0f) {
			for (j = -50; j < 50; j += 10.0f) {
				//for (k = -50; k < 50; k += 10.0f) {
				Entity osws = mWorld->CreateEntity();	// �ʼ�
				t.mLocalPosition = { i * n, 0, j * n };


				mWorld->AddComponent<TransformComponent>(osws, t);
				mWorld->AddComponent<RenderComponent>(osws, phereMesh, material2s);
				mWorld->AddComponent<GravityComponent>(osws);
				//}
			}

		}
	}


	/////////////////////////////////////////////////////////////////////////



#pragma region UI
	{
		Entity hpBAR = mWorld->CreateEntity();
		shared_ptr<Material> scorem;
		scorem = RESOURCEMANAGER.Get<Material>(L"HPBAR");

		auto& t = mWorld->AddComponent<UITransformComponent>(hpBAR);
		t.mAnchor = Anchor::Center;
		t.mPosition = Vec2(-256.f, 768.f);
		t.mSize = Vec2(512.f, 256.f);


		auto& m = mWorld->AddComponent<UISpriteComponent>(hpBAR, scorem);
	}

	{
		Entity Aim = mWorld->CreateEntity();
		shared_ptr<Material> scorem;
		scorem = RESOURCEMANAGER.Get<Material>(L"Aim");

		auto& t = mWorld->AddComponent<UITransformComponent>(Aim);
		t.mAnchor = Anchor::Center;
		t.mPosition = Vec2(-64.f, -64.f);
		t.mSize = Vec2(128.f, 128.f);

		auto& m = mWorld->AddComponent<UISpriteComponent>(Aim, scorem);
	}

	{
		Entity Ibanix_Ammo = mWorld->CreateEntity();
		shared_ptr<Material> scorem;
		scorem = RESOURCEMANAGER.Get<Material>(L"Ibanix_Ammo");

		auto& t = mWorld->AddComponent<UITransformComponent>(Ibanix_Ammo);
		t.mAnchor = Anchor::BottomLeft;
		t.mPosition = Vec2(212.f, -212.f);
		t.mSize = Vec2(196.f, 128.f);

		auto& m = mWorld->AddComponent<UISpriteComponent>(Ibanix_Ammo, scorem);
	}

	{
		Entity Fanthor_Portrait = mWorld->CreateEntity();

		shared_ptr<Material> scorem;
		scorem = RESOURCEMANAGER.Get<Material>(L"Fanthor_Portrait");

		auto& t = mWorld->AddComponent<UITransformComponent>(Fanthor_Portrait);
		t.mAnchor = Anchor::BottomLeft;
		t.mPosition = Vec2(32.f, -636.f);
		t.mSize = Vec2(196.f, 196.f);

		auto& m = mWorld->AddComponent<UISpriteComponent>(Fanthor_Portrait, scorem);
	}

	{
		Entity Ibanix_Portrait = mWorld->CreateEntity();

		shared_ptr<Material> scorem;
		scorem = RESOURCEMANAGER.Get<Material>(L"Ibanix_Portrait");

		auto& t = mWorld->AddComponent<UITransformComponent>(Ibanix_Portrait);
		t.mAnchor = Anchor::BottomLeft;
		t.mPosition = Vec2(32.f, -424.f);
		t.mSize = Vec2(196.f, 196.f);

		auto& m = mWorld->AddComponent<UISpriteComponent>(Ibanix_Portrait, scorem);
	}

	{
		Entity Rudwig_Portrait = mWorld->CreateEntity();

		shared_ptr<Material> scorem;
		scorem = RESOURCEMANAGER.Get<Material>(L"Rudwig_Portrait");

		auto& t = mWorld->AddComponent<UITransformComponent>(Rudwig_Portrait);
		t.mAnchor = Anchor::BottomLeft;
		t.mPosition = Vec2(32.f, -212.f);
		t.mSize = Vec2(196.f, 196.f);

		auto& m = mWorld->AddComponent<UISpriteComponent>(Rudwig_Portrait, scorem);
	}

#pragma endregion

	/////////////////////////////////////////////////////////////////////////



	mWorld->Initialize();
}

void GameScene::Update(float deltaTime)
{
	mWorld->Update(deltaTime);
}

void GameScene::Render()
{
	mWorld->Render();
}
