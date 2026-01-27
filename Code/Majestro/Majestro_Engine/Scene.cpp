#include "pch.h"
#include "Scene.h"
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
	//PlayerPrefab player{mWorld.get()};
	PrefabFactory::RegisterAllPrefabs();
	TerrainPrefab terrain{ mWorld.get() };
	SkyBoxPrefab skybox{ mWorld.get() };
	DirLightPrefab light{ mWorld.get() };
	EnemyPrefab	enemys {mWorld.get() };
	LoadJsonScene(L"..\\Resources\\Json\\MajestroScene.json");

	
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
					t.mLocalPosition = { i*n, 0, j*n };


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
		t.mAnchor = Anchor::TopLeft;
		t.mPosition = Vec2(50.f, 0.f);
		t.mSize = Vec2(512.f, 256.f);


		auto& m = mWorld->AddComponent<UISpriteComponent>(hpBAR,scorem);
	}
	//{
	//	Entity Bass = mWorld->CreateEntity();

	//	shared_ptr<Material> scorem;
	//	scorem = RESOURCEMANAGER.Get<Material>(L"BassPortrait");

	//	auto& t = mWorld->AddComponent<UITransformComponent>(Bass);
	//	t.mAnchor = Anchor::TopLeft;
	//	t.mPosition = Vec2(750.f, 250.f);
	//	t.mSize = Vec2(300.f, 500.f);
	//	


	//	auto& m = mWorld->AddComponent<UISpriteComponent>(Bass, scorem);
	//}
	{
		Entity Bass = mWorld->CreateEntity();

		shared_ptr<Material> scorem;
		scorem = RESOURCEMANAGER.Get<Material>(L"GuitarPortrait");

		auto& t = mWorld->AddComponent<UITransformComponent>(Bass);
		t.mAnchor = Anchor::TopLeft;
		t.mPosition = Vec2(280.f, 110.f);
		t.mSize = Vec2(512.f, 256.f);
		


		auto& m = mWorld->AddComponent<UISpriteComponent>(Bass, scorem);
	}

	{
		Entity osw = mWorld->CreateEntity();
		shared_ptr<Mesh> phereMesh = RESOURCEMANAGER.Get<Mesh>(L"Rock_Overgrown_I_LOD00");
		std::vector<shared_ptr<Material>> material2s;


		shared_ptr<Texture> texture = RESOURCEMANAGER.Load<Texture>(L"ROCK", L"..\\Resources\\Texture\\ROCK.png");
		shared_ptr<Texture> texture2 = RESOURCEMANAGER.Load<Texture>(L"ROCKs", L"..\\Resources\\Texture\\Leather_Normal.jpg");
		shared_ptr<Texture> texture3 = RESOURCEMANAGER.Load<Texture>(L"ROCK2", L"..\\Resources\\Texture\\BakedShaderGraph_Occlusion.png");
		shared_ptr<Material> material = make_shared<Material>();
		material->SetShader(L"Deferred");
		material->SetTexture(texture, DIFFUSEMAP0INDEX);
		material->SetTexture(texture, DIFFUSEMAP1INDEX);
		material->SetTexture(texture, DIFFUSEMAP2INDEX);
		material->SetTexture(texture2, NORMALMAPINDEX);
		//material->SetTexture(texture3, OCCLUSIONMAPINDEX);


		RESOURCEMANAGER.Add<Material>(L"ROCK", material);

		shared_ptr<Material> material2 = RESOURCEMANAGER.Get<Material>(L"ROCK");
		material2s.push_back(material2);
		TransformComponent t{};
		t.mLocalPosition = { 0.f, 0.f, 0.f };
		t.mLocalScale = { 1.f, 1.f, 1.f };


		mWorld->AddComponent<TransformComponent>(osw, t);
		mWorld->AddComponent<RenderComponent>(osw, phereMesh, material2s);
		//mWorld->AddComponent<AnimationComponent>(osw, anmators);
	
	}

#pragma endregion

	/////////////////////////////////////////////////////////////////////////


	
	mWorld->Initialize();
}

void Scene::Update(float deltaTime)
{
	mWorld->Update(deltaTime);
}

void Scene::Render()
{
	mWorld->Render();
}

void Scene::LoadJsonScene(const wstring& path)
{
	
	SceneMapDesc sceneData = RESOURCEMANAGER.ImportUnityScene(path);

	for (auto& model : sceneData.objects)
	{

		if (model.active)
		{
			Entity entity = mWorld->CreateEntity();
			shared_ptr<FBXData> data = RESOURCEMANAGER.LoadJsonFbx(s2ws("..\\Resources\\FBX\\"+model.meshFbxFile));

			// model.meshFbxFile로 부터 파일 이름만 추출
			//std::string meshName = filesystem::path(model.meshFbxFile).stem().string();
			shared_ptr<Mesh> mesh = RESOURCEMANAGER.Get<Mesh>(s2ws("__FBXEXPORT__" + model.meshName));
			if(mesh == nullptr)
			{
				std::cout << "Mesh not found: " << model.meshName << std::endl;
				assert(false);
			}
			for(auto& mat : model.materials)
			{
				shared_ptr<Material> material = RESOURCEMANAGER.Get<Material>(s2ws(model.meshName));

				std::vector<shared_ptr<Material>> materials;
				if(material == nullptr) {
					material = make_shared<Material>();
					material->SetShader(L"Deferred");
					if (mat.albedo != "") {
						std::string meshName = filesystem::path(mat.albedo).stem().string();
						shared_ptr<Texture> texture = RESOURCEMANAGER.Load<Texture>(s2ws(meshName), s2ws("..\\Resources\\Texture\\" + mat.albedo));
						RESOURCEMANAGER.Add<Texture>(s2ws(mat.albedo), texture);
						material->SetTexture(texture, DIFFUSEMAP0INDEX);
						material->SetTexture(texture, DIFFUSEMAP1INDEX);
					}
					if (mat.normal != "") {
						std::string meshName = filesystem::path(mat.normal).stem().string();
						shared_ptr<Texture> texture = RESOURCEMANAGER.Load<Texture>(s2ws(meshName), s2ws("..\\Resources\\Texture\\" + mat.normal));
						RESOURCEMANAGER.Add<Texture>(s2ws(mat.normal), texture);
						material->SetTexture(texture, NORMALMAPINDEX);
					}
					/*	if (mat.smoothness !="") {
							shared_ptr<Texture> texture = RESOURCEMANAGER.Load<Texture>(s2ws(mat.smoothness), s2ws(mat.smoothness));
							RESOURCEMANAGER.Add<Texture>(s2ws(mat.smoothness), texture);
							material->SetTexture(texture, ROUGHNESSMAPINDEX);
						}*/
					if (mat.metallic != "") {
						std::string meshName = filesystem::path(mat.metallic).stem().string();
						shared_ptr<Texture> texture = RESOURCEMANAGER.Load<Texture>(s2ws(meshName), s2ws("..\\Resources\\Texture\\" + mat.metallic));
						RESOURCEMANAGER.Add<Texture>(s2ws(mat.metallic), texture);
						material->SetTexture(texture, METALLICMAPINDEX);
					}
					if (mat.occlusion != "") {
						std::string meshName = filesystem::path(mat.occlusion).stem().string();
						shared_ptr<Texture> texture = RESOURCEMANAGER.Load<Texture>(s2ws(meshName), s2ws("..\\Resources\\Texture\\" + mat.occlusion));
						RESOURCEMANAGER.Add<Texture>(s2ws(mat.occlusion), texture);
						material->SetTexture(texture, OCCLUSIONMAPINDEX);
					}
					RESOURCEMANAGER.Add<Material>(s2ws(model.meshName), material);
				}
				
				materials.push_back(material);
				mWorld->AddComponent<RenderComponent>(entity, mesh, materials);
				
			}
			mWorld->AddComponent<TransformComponent>(entity,  model.pos, model.rotEulerDeg, model.scale/27 );

		}
	}
}


