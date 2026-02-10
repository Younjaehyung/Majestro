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

void Scene::Shudown()
{
	mWorld->Shutdown();

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
		Entity testCamera = mWorld->CreateEntity();
		TransformComponent t{};
		t.mLocalPosition = { 100.f, 150.f, 300.f };
		t.mLocalRotationE = { 20.f, 180.f, 0.f };
		mWorld->AddComponent<MainCameraComponent>(testCamera);
		mWorld->AddComponent<CameraComponent>(testCamera);
		mWorld->AddComponent<TransformComponent>(testCamera, t);
	}

	{
		Entity mannequinEntity = mWorld->CreateEntity();
		mWorld->AddComponent<ChoicePlayerComponent>(mannequinEntity, 1);
		
	}
	
	for (int i = 0;i < 3;++i) {
		Entity mEntityID = mWorld->CreateEntity();

		TransformComponent t{};
		shared_ptr<Mesh> phereMesh;
		shared_ptr<Material> material2;
		std::vector<shared_ptr<Material>> material2s;
		vector<shared_ptr<Animator>> anmators0;

		switch (i) {
		case 0:
			phereMesh = RESOURCEMANAGER.Get<Mesh>(L"SM_Rudwig_Body");
			material2 = RESOURCEMANAGER.Get<Material>(L"Anim_Rudwig_Idle0");
			material2s.push_back(material2);
			anmators0.push_back(RESOURCEMANAGER.Get<Animator>(L"Anim_Rudwig_Idle"));
			anmators0.push_back(RESOURCEMANAGER.Get<Animator>(L"Anim_Rudwig_Run"));
			mWorld->AddComponent<MannequinComponent>(mEntityID,i);
			break;
		case 1:
			phereMesh = RESOURCEMANAGER.Get<Mesh>(L"SM_Ibanix_Body");
			material2 = RESOURCEMANAGER.Get<Material>(L"Anim_Ibanix_Idle0");
			material2s.push_back(material2);
			anmators0.push_back(RESOURCEMANAGER.Get<Animator>(L"Anim_Ibanix_Idle"));
			anmators0.push_back(RESOURCEMANAGER.Get<Animator>(L"Anim_Ibanix_Walk"));
			mWorld->AddComponent<MannequinComponent>(mEntityID, i);
			break;
		case 2:
			phereMesh = RESOURCEMANAGER.Get<Mesh>(L"SM_Fanthor_Body");
			material2 = RESOURCEMANAGER.Get<Material>(L"Anim_Fanthor_Idle0");
			material2s.push_back(material2);
			anmators0.push_back(RESOURCEMANAGER.Get<Animator>(L"Anim_Fanthor_Idle"));
			anmators0.push_back(RESOURCEMANAGER.Get<Animator>(L"Anim_Fanthor_Walk"));
			mWorld->AddComponent<MannequinComponent>(mEntityID, i);
			break;
		}

		t.mLocalPosition = { i * 100.f, 0.f, 0.f };
		mWorld->AddComponent<TransformComponent>(mEntityID, t);
		mWorld->AddComponent<RenderComponent>(mEntityID, phereMesh, material2s);
		mWorld->AddComponent<AnimationComponent>(mEntityID, anmators0);
	}
	

	// MAP export json load
	// [참고] 현재 FBX LOADER에서 NormalMap을 읽지 못하게 함.
	// 

	
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





	/////////////////////////////////////////////////////////////////////////


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
	 //LoadJsonLevel(L"..\\Resources\\Json\\M_StylizedStudyLogCabin_A1_Export.json");
	// LoadJsonLevel(L"..\\Resources\\Json\\ThirdPersonMap_Export.json");

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
		Entity enityt = mWorld->CreateEntity();
		TransformComponent t{};
		t.mLocalPosition = Vec3(1500.f, 720.f, 0.f);
		mWorld->AddComponent<TransformComponent>(enityt, t);
		shared_ptr<Mesh> mesh = RESOURCEMANAGER.Get<Mesh>(L"SM_Rudwig_Body");
		shared_ptr<Material> material = RESOURCEMANAGER.Get<Material>(L"Anim_Rudwig_Idle0");
		std::vector<shared_ptr<Material>> materials;
		materials.push_back(material);
		mWorld->AddComponent<RenderComponent>(enityt, mesh,  materials);

	}
	{
		/*Entity enityt = mWorld->CreateEntity();
		TransformComponent t{};
		t.mLocalPosition = Vec3(-120.f, 720.f, 0.f);
		t.mLocalRotationE = Vec3(0.f, 90.f, 0.f);
			t.mLocalScale = Vec3(15.f, 15.f, 15.f);
		mWorld->AddComponent<TransformComponent>(enityt, t);
		shared_ptr<Mesh> mesh = RESOURCEMANAGER.Get<Mesh>(L"Cube");
		shared_ptr<Material> material = RESOURCEMANAGER.Get<Material>(L"XYZ0");
		std::vector<shared_ptr<Material>> materials;
		materials.push_back(material);
		mWorld->AddComponent<RenderComponent>(enityt, mesh,  materials);*/

	}

	{
		/*Entity enityt = mWorld->CreateEntity();
		TransformComponent t{};
		t.mLocalPosition = Vec3(0.f, 720.f, 0.f);
		mWorld->AddComponent<TransformComponent>(enityt, t);
		shared_ptr<Mesh> mesh = RESOURCEMANAGER.Get<Mesh>(L"SM_Rock_04");
		shared_ptr<Material> material = RESOURCEMANAGER.Get<Material>(L"ZUP_Ascii_3dmax_Pivot0");
		std::vector<shared_ptr<Material>> materials;
		materials.push_back(material);
		mWorld->AddComponent<RenderComponent>(enityt, mesh, materials);*/

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
			std::string name = filesystem::path(inst.fbx).filename().stem().string();
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
			transform.mWorldMatrix = inst.worldMtx;

			TransformComponent& trans = mWorld->AddComponent<TransformComponent>(entity, transform);
			trans.mIsStatic = true;

			RenderComponent& render = mWorld->AddComponent<RenderComponent>(entity);

			//for (const auto& mat : data->GetMaterials()) {
			//	mat->SetTexture(RESOURCEMANAGER.Get<Texture>(L"T_Rock_BC"), DIFFUSEMAP0INDEX);
			//}
			render.mMaterials = data->GetMaterials();

			render.mMesh = data->GetMeshs().at(0);
			

		}
	}
	catch (const std::exception& e)
	{
		std::cerr << "Load failed: " << e.what() << "\n";
	}
}

