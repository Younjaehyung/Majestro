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
#include "UITextComponent.h"
#include "BeatComponent.h"
#include "GravityComponent.h"
#include "MovementComponent.h"
#include "NetSendSystem.h"
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
		mWorld->AddComponent<ChoicePlayerComponent>(mannequinEntity, 0);
		//mWorld->AddComponent<PlayerMovementComponent>(mannequinEntity);
		
	}


	// MAP export json load
	// [참고] 현재 FBX LOADER에서 NormalMap을 읽지 못하게 함.
	// 
	 //LoadJsonLevel(L"..\\Resources\\Json\\M_StylizedStudyLogCabin_A1_Export.json");


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
		Entity text = mWorld->CreateEntity();
		auto& t = mWorld->AddComponent<UITextComponent>(text);
	}


	{
		Entity hpBAR = mWorld->CreateEntity();
		shared_ptr<Material> scorem;
		scorem = RESOURCEMANAGER.Get<Material>(L"HPBAR");

		auto& t = mWorld->AddComponent<UITransformComponent>(hpBAR);
		t.mAnchor = Anchor::Center;
		t.mPosition = Vec2(-256.f, 468.f);
		t.mSize = Vec2(512.f, 256.f);


		auto& m = mWorld->AddComponent<UISpriteComponent>(hpBAR, scorem);
	}

	{
		Entity Aim = mWorld->CreateEntity();
		shared_ptr<Material> scorem;
		scorem = RESOURCEMANAGER.Get<Material>(L"jAims");

		auto& t = mWorld->AddComponent<UITransformComponent>(Aim);
		t.mAnchor = Anchor::Center;
		t.mPosition = Vec2(-98.f, -64.f);
		t.mSize = Vec2(196.f, 128.f);

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

// MAP export json load
// [참고] 현재 FBX LOADER에서 NormalMap을 읽지 못하게 함.
// 
LoadJsonLevel(L"..\\Resources\\Json\\M_StylizedStudyLogCabin_A1_Export.json");

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



void Scene::LoadJsonLevel(const wstring& path)
{
	//RESOURCEMANAGER.Load<Texture>(L"normalgun", L"..\\Resources\\Texture\\MI_Trims_C_DarkGray_Normal_0.png");
	int i = 0;
	try
	{
		LevelImportData level = RESOURCEMANAGER.LoadResourceJson(path);

		for (const auto& inst : level.instances)
		{
			// 파일명만 추출
			std::string name = filesystem::path(inst.staticMeshAsset).filename().stem().string();
			name = "..\\Resources\\FBX\\" + name + ".fbx";
			shared_ptr<FBXData> data = RESOURCEMANAGER.LoadFBXMesh(s2ws(name));

			if (!data)
			{
				std::cerr << "FBX load failed (null data): " << name << "\n";
				break;
			}
			else if (data->GetMaterials().empty()) {
				std::cerr << "FBX load failed Material (null data): " << name << "\n";
				continue;
			}
			else if (data->GetMeshs().empty()) {
				std::cerr << "FBX load failed Mesh (null data): " << name << "\n";
				continue;
			}

			Entity entity = mWorld->CreateEntity();
			TransformComponent transform{};
			transform.mLocalScale = Vec{ 0.1f, 0.1f, 0.1f };
			transform.FinalUpdate();
			//Matrix instanceMatrix = BuildWorldMatrix_RowMajor(inst.world, true);
			Matrix worldMatrix = inst.worldMtx;
			transform.mWorldMatrix = worldMatrix;// *transform.mWorldMatrix;

			TransformComponent& trans = mWorld->AddComponent<TransformComponent>(entity, transform);
			trans.mIsStatic = true;

			RenderComponent& render = mWorld->AddComponent<RenderComponent>(entity);

			/*for (const auto& mat : data->GetMaterials()) {
				mat->SetTexture(RESOURCEMANAGER.Get<Texture>(L"normalgun"), NORMALMAPINDEX);
			}*/
			render.mMaterials = data->GetMaterials();
			render.mMesh = data->GetMeshs().at(0);

			i++;
			if (i == 800)break;

		}
	}
	catch (const std::exception& e)
	{
		std::cerr << "Load failed: " << e.what() << "\n";
	}
}

