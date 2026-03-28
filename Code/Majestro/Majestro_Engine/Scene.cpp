#include "pch.h"
#include "Scene.h"
#include "Engine.h"
#include "SceneManager.h"
#include "EnginePch.h"
#include "RenderManager.h"
#include "ResourceManager.h"
#include "AudioManager.h"
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
#include "UIComponent.h"
#include "UIVfxComponent.h"
#include "UITextComponent.h"
#include "BeatComponent.h"
#include "GravityComponent.h"
#include "MovementComponent.h"
#include "NetSendSystem.h"
#include "VfxComponent.h"
#include "BoxColliderComponent.h"
#include "AudioVisualizerComponent.h"
#include "EffectFlagComponent.h"
#include "Prefab.h"

#include "RenderSystem.h"
#include "GameRenderPipeline.h"
#include "LobbyRenderPipeline.h"
#include "CameraSystem.h"
#include "AudioSystem.h"
#include "TransformSystem.h"
#include "AnimationSystem.h"
#include "PlayerSystem.h"
#include "UIRenderSystem.h"
#include "UIUpdateSystem.h"
#include "IMGUISystem.h"
#include "BeatSystem.h"
#include "MovementSystem.h"

#include "NetRecvSystem.h"
#include "NetSendSystem.h"
#include "PlayerInputSystem.h"
#include "EnemySystem.h"
#include "CollisionSystem.h"
#include "NetInterpolationSystem.h"
#include "AudioVisualizerSystem.h"
#include "InputManager.h"
#include "GameMode.h"
#include "UIButtonComponent.h"
#include "UIButtonSystem.h"


Scene::Scene()
{

}

void Scene::Initialize()
{
	
}

void Scene::Update(float deltaTime)
{
	//std::cerr << "Scene Update: " << (int32)mSceneId;
	mGameMode->PreUpdate(deltaTime);
	mWorld->Update(deltaTime);
	mGameMode->PostUpdate(deltaTime);
}

void Scene::Render()
{
	mWorld->Render();
}

void Scene::Shudown()
{
	mWorld->Shutdown();

}

void Scene::LoadJsonLevel(const wstring& path)
{

	int i = 0;
	try
	{
		LevelImportData level = RESOURCEMANAGER.LoadResourceJson(path);

		for (const auto& inst : level.instances)
		{
			// 파일명만 추출
			std::string name = filesystem::path(inst.fbx).filename().stem().string();
			name = "..\\Resources\\Map\\" + name + ".fbx";
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
			render.mCheckFrustum = false;
			render.mMesh = data->GetMeshs().at(0);
			i++;
			/*		if (i == 550)
						break;*/
						/*BoxColliderComponent& boxCollider = mWorld->AddComponent<BoxColliderComponent>(entity,
							data->GetColliders().at(0)->GetOBB(), transform.mWorldMatrix);


						mWorld->GetPhysicsWorld()->AddStaticOBB(entity, boxCollider.mWorldOBB, 0);
			*/


		}
	}
	catch (const std::exception& e)
	{
		std::cerr << "Load failed: " << e.what() << "\n";
	}
}

void Scene::LoadCollisionJson(const wstring& path)
{
	int i = 0;
	try
	{
		LevelImportData level = RESOURCEMANAGER.LoadResourceJson(path);

		for (const auto& inst : level.instances)
		{
			if (std::string::npos != inst.fbx.find("CRX_Sphere")) {
				std::cout << "A" << std::endl;
			}

			if (std::string::npos == inst.fbx.find("CRX_Cube"))
				continue;


			// 파일명만 추출
			shared_ptr<Mesh> data = RESOURCEMANAGER.LoadMCubeMesh();
			//BoundingOrientedBox obb = BoundingOrientedBox(Vec3(0.f, 0.f, 0.f), Vec3(50.f, 50.f, 50.f), Quaternion::Identity);


			Entity entity = mWorld->CreateEntity();
			TransformComponent transform{};
			transform.mWorldMatrix = inst.worldMtx;

			TransformComponent& trans = mWorld->AddComponent<TransformComponent>(entity, transform);
			trans.mIsStatic = true;


#ifdef _DEBUG
			RenderComponent& render = mWorld->AddComponent<RenderComponent>(entity);
			std::vector<std::shared_ptr<Material>> materials;
			materials.push_back(RESOURCEMANAGER.Get<Material>(L"Skybox"));
			render.mMaterials = materials;
			render.mCheckFrustum = false;
			render.mMesh = data;
#endif


			/*BoxColliderComponent& boxCollider = mWorld->AddComponent<BoxColliderComponent>(entity,
				obb, transform.mWorldMatrix);*/


				//mWorld->GetPhysicsWorld()->AddStaticOBB(entity, boxCollider.mWorldOBB, 0);
			++i;
			std::cout << i << std::endl;

		}
	}
	catch (const std::exception& e)
	{
		std::cerr << "Load failed: " << e.what() << "\n";
	}
}

void Scene::SetGameMode(shared_ptr<GameMode>& gameMode)
{
	if (gameMode) {
		mGameMode = gameMode;

		mGameMode->SetScene(shared_from_this()); // GameMode에 씬 참조 전달
	}
	
}

//////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////

#pragma region Menu Scenes

void MainMenuScene::Initialize()
{
	mWorld->SetSceneId(mSceneId);
	//PlayerPrefab player{mWorld.get()};
	PrefabFactory::RegisterAllPrefabs();
	TerrainPrefab terrain{ mWorld.get() };
	SkyBoxPrefab skybox{ mWorld.get() };
	DirLightPrefab light{ mWorld.get() };

// [참고] 현재 FBX LOADER에서 NormalMap을 읽지 못하게 함.
// 
	//LoadJsonLevel(L"..\\Resources\\Json\\M_StylizedStudyLogCabin_A1_Export.json");
	// LoadJsonLevel(L"..\\Resources\\Json\\ThirdPersonMap_Export.json");
	LoadJsonLevel(L"..\\Resources\\Json\\Map001_Export.json");
	//LoadCollisionJson(L"..\\Resources\\Json\\Map001_CRX.json");


	/////////////////////////////////////////////////////////////////////////
	{
		Entity testCamera = mWorld->CreateEntity();
		TransformComponent t{};
		t.mLocalPosition = { -7944.051237f,  1880.474238f, -13680.832254f };
		t.mLocalRotationE = { -3.400000f, -81.999999f, 0.f };
		mWorld->AddComponent<MainCameraComponent>(testCamera);
		mWorld->AddComponent<CameraComponent>(testCamera);
		mWorld->AddComponent<TransformComponent>(testCamera, t);
	}

	/////////////////////////////////////////////////////////////////////////
	// 메인메뉴 버튼 UI
	/////////////////////////////////////////////////////////////////////////
	{
		const Vec2  btnSize  = { 320.f, 72.f };
		const float startX	 =  850.f;   // 화면 중앙 기준 X 오프셋 (위쪽 버튼)
		const float startY   =  270.f;   // 화면 중앙 기준 Y 오프셋 (위쪽 버튼)
		const float gap      = 100.f;

		// 버튼 생성 헬퍼 람다
		auto MakeButton = [&](const wchar_t* name, const wchar_t* label,
		                      float offsetY, std::function<void()> onClick)
		{
			// 버튼별 머티리얼 생성 후 ResourceManager에 등록 (GPU 인덱스 필수)
			auto mat = RESOURCEMANAGER.Get<Texture>(name);

			Entity e = mWorld->CreateEntity();

			// 위치·크기
			auto& tr     = mWorld->AddComponent<UITransformComponent>(e);
			tr.mAnchor   = Anchor::Center;
			tr.mPosition = Vec2(0.f, offsetY);
			tr.mSize     = btnSize;
			tr.mPivot    = Vec2(0.5f, 0.5f);
			tr.mUILayerIndex = 5;

			// 스프라이트 (색상 박스)
			mWorld->AddComponent<UISpriteComponent>(e, mat);

			// 텍스트 레이블 (UIButtonSystem이 매 프레임 mFontPos 갱신)
			auto& txt  = mWorld->AddComponent<UITextComponent>(e);
			txt.mText  = label;

			// 버튼 컴포넌트
			auto& btn      = mWorld->AddComponent<UIButtonComponent>(e);
			btn.mBaseSize  = btnSize;
			btn.mOnClick   = std::move(onClick);

			// 초기 색상 설정
			//mat->GetParams().Diffuse = btn.mNormalColor;
		};


		auto MakeVFXButton = [&](const wchar_t* name, const wchar_t* label,
			Vec2 offset, std::function<void()> onClick)
			{
				// 버튼별 머티리얼 생성 후 ResourceManager에 등록 (GPU 인덱스 필수)
				auto mat = RESOURCEMANAGER.Get<Vfx>(name);

				Entity e = mWorld->CreateEntity();

				// 위치·크기
				auto& tr = mWorld->AddComponent<UITransformComponent>(e);
				tr.mAnchor = Anchor::Center;
				tr.mPosition = offset;
				tr.mSize = btnSize;
				tr.mPivot = Vec2(0.5f, 0.5f);
				tr.mUILayerIndex = 5;

				// 스프라이트 (색상 박스)
				mWorld->AddComponent<UIVfxComponent>(e, mat, true, 100.f);

				// 텍스트 레이블 (UIButtonSystem이 매 프레임 mFontPos 갱신)
				auto& txt = mWorld->AddComponent<UITextComponent>(e);
				txt.mText = label;

				// 버튼 컴포넌트
				auto& btn = mWorld->AddComponent<UIButtonComponent>(e);
				btn.mBaseSize = btnSize;
				btn.mOnClick = std::move(onClick);
				btn.mVfxNormalScale = 100.f;
				btn.mVfxHoveredScale = 115.f;
				btn.mVfxPressedScale = 90.f;

				// 초기 색상 설정
				//mat->GetParams().Diffuse = btn.mNormalColor;

				return e;
			};

		// ── 게임 시작 ──
		Entity e1 = MakeVFXButton(L"UI_TItle", L"GAMESTART", Vec2(startX,startY), [&]()
		{
				Network::GetInstance().Awake();
				mGameMode->mTargetSceneId = SceneId::Lobby;
				mGameMode->IsSceneChanging() = true;
		});

#ifdef _IMGUI

		
		UITransformComponent* vis = mWorld->GetComponent<UITransformComponent>(e1);
		std::vector<EditorProperty> props;
		props.push_back({ "Base Position1",  PropertyType::Vec2,  &(vis->mPosition),  0.f,    0.f });
		
#endif

		// ── 설정 (미구현 플레이스홀더) ──
		Entity e2 = MakeVFXButton(L"UI_TItle", L"SETTING", Vec2(startX, startY + gap), []()
		{
			// TODO: 설정 씬 또는 팝업 구현 후 연결
		});

#ifdef _IMGUI

		vis = mWorld->GetComponent<UITransformComponent>(e2);
		props.push_back({ "Base Position2",  PropertyType::Vec2,  &(vis->mPosition),  0.f,    0.f });
#endif

		// ── 나가기 ──
		Entity e3 = MakeVFXButton(L"UI_TItle", L"EXIT", Vec2(startX, startY +gap * 2.f), []()
		{
			PostQuitMessage(0);
		});

#ifdef _IMGUI
		vis = mWorld->GetComponent<UITransformComponent>(e3);
		props.push_back({ "Base Position3",  PropertyType::Vec2,  &(vis->mPosition),  0.f,    0.f });
		IMGUIComponent& visImgui = mWorld->AddComponent<IMGUIComponent>(e1);
		visImgui.RegisterEditorProperties(props);
		visImgui.SetName("Menu");
#endif
	}


	for (int i = 0; i < 3; ++i) {
		Entity mEntityID = mWorld->CreateEntity();

		TransformComponent t{};
		shared_ptr<Mesh> phereMesh;
		shared_ptr<Material> material2;
		std::vector<shared_ptr<Material>> material2s;
		vector<shared_ptr<Animator>> anmators0;

		switch (i) {
		case 0:
			phereMesh = RESOURCEMANAGER.Get<Mesh>(L"SM_Rudwig_Body");
			material2 = RESOURCEMANAGER.Get<Material>(L"Anim_Rudwig_Attack_010")->Clone();
			RESOURCEMANAGER.Add<Material>(L"Anim_Rudwig_Attack_010S", material2);
			material2 = RESOURCEMANAGER.Get<Material>(L"Anim_Rudwig_Attack_010S");
			material2->SetShader(L"Solid");
			material2->GetParams().ExtValue[0] = Vec4(0.f, 0.8f, 0.f, 1.f); // RimPower
			material2s.push_back(material2);


			material2 = RESOURCEMANAGER.Get<Material>(L"Anim_Rudwig_Attack_011")->Clone();
			RESOURCEMANAGER.Add<Material>(L"Anim_Rudwig_Attack_011S", material2);
			material2 = RESOURCEMANAGER.Get<Material>(L"Anim_Rudwig_Attack_011S");
			material2->SetShader(L"Solid");
			material2->GetParams().ExtValue[0] = Vec4(0.f, 0.5f, 0.5f, 1.f); // RimPower
			material2s.push_back(material2);

			anmators0.push_back(RESOURCEMANAGER.Get<Animator>(L"Anim_Rudwig_Idle"));
			anmators0.push_back(RESOURCEMANAGER.Get<Animator>(L"Anim_Rudwig_Run"));
			//mWorld->AddComponent<MannequinComponent>(mEntityID, i);
			t.mLocalPosition = { -8703.f, 1711.0f,-13849.0f };
			break;
		case 1:
			phereMesh = RESOURCEMANAGER.Get<Mesh>(L"SM_Ibanix_Body");
			material2 = RESOURCEMANAGER.Get<Material>(L"Anim_Ibanix_Attack_010")->Clone();
			RESOURCEMANAGER.Add<Material>(L"Anim_Ibanix_Attack_010S", material2);
			material2 = RESOURCEMANAGER.Get<Material>(L"Anim_Ibanix_Attack_010S");
			material2->SetShader(L"Solid");
			material2->GetParams().ExtValue[0] = Vec4(0.f, 0.f, 8.f, 1.f); // RimPower
			material2s.push_back(material2);



			material2 = RESOURCEMANAGER.Get<Material>(L"Anim_Ibanix_Attack_011")->Clone();
			RESOURCEMANAGER.Add<Material>(L"Anim_Ibanix_Attack_011S", material2);
			material2 = RESOURCEMANAGER.Get<Material>(L"Anim_Ibanix_Attack_011S");
			material2->SetShader(L"Solid");
			material2->GetParams().ExtValue[0] = Vec4(0.5f, 0.f, 5.f, 1.f); // RimPower
			material2s.push_back(material2);
			anmators0.push_back(RESOURCEMANAGER.Get<Animator>(L"Anim_Ibanix_Idle"));
			anmators0.push_back(RESOURCEMANAGER.Get<Animator>(L"Anim_Ibanix_Walk"));
			//mWorld->AddComponent<MannequinComponent>(mEntityID, i);
			t.mLocalPosition = { -9171.0f, 1711.0f, -13372.f };
			break;
		case 2:
			phereMesh = RESOURCEMANAGER.Get<Mesh>(L"SM_Fanthor_Body");
			material2 = RESOURCEMANAGER.Get<Material>(L"Anim_Fanthor_Attack_010")->Clone();
			RESOURCEMANAGER.Add<Material>(L"Anim_Fanthor_Attack_010S", material2);
			material2 = RESOURCEMANAGER.Get<Material>(L"Anim_Fanthor_Attack_010S");
			material2->GetParams().ExtValue[0] = Vec4(0.8f, 0.8f, 0.f, 1.f); // RimPower
			material2->SetShader(L"Solid");
			material2s.push_back(material2);


			material2 = RESOURCEMANAGER.Get<Material>(L"Anim_Fanthor_Attack_011")->Clone();
			RESOURCEMANAGER.Add<Material>(L"Anim_Fanthor_Attack_011S", material2);
			material2 = RESOURCEMANAGER.Get<Material>(L"Anim_Fanthor_Attack_011S");
			material2->SetShader(L"Solid");
			material2->GetParams().ExtValue[0] = Vec4(0.f,0.5f,0.5f,1.f); // RimPower
			material2s.push_back(material2);
			anmators0.push_back(RESOURCEMANAGER.Get<Animator>(L"Anim_Fanthor_Idle"));
			anmators0.push_back(RESOURCEMANAGER.Get<Animator>(L"Anim_Fanthor_Walk"));
			//mWorld->AddComponent<MannequinComponent>(mEntityID, i);
			t.mLocalPosition = { -8834.f, 1711.0f,-12656.0f };
			break;
		}

		
		mWorld->AddComponent<TransformComponent>(mEntityID, t);
		mWorld->AddComponent<RenderComponent>(mEntityID, phereMesh, material2s);
		mWorld->AddComponent<AnimationComponent>(mEntityID, anmators0);
		/*EffectFlagComponent& effect = mWorld->AddComponent<EffectFlagComponent>(mEntityID);
		effect.mFlag = EffectFlag::Infinite;*/
	}

	//-- -
	//	UIVfxComponent 사용 방법
	//	위치는 반드시 UITransformComponent로 설정해야 함
	//
	//	auto& tr = mWorld->AddComponent<UITransformComponent>(e);
	//	tr.mAnchor   = Anchor::Center;
	//	tr.mPosition = Vec2(0.f, -60.f);
	//	tr.mSize     = Vec2(300.f, 80.f);  // 히트테스트 영역 (버튼이면)
	//	tr.mPivot    = Vec2(0.5f, 0.5f);
	//
	//	auto& vfx = mWorld->AddComponent<UIVfxComponent>(e);
	//	vfx.mVfx    = RESOURCEMANAGER.Get<Vfx>(L"MenuEffect");
	//	vfx.mIsLoop = true;
	//
	//	// 버튼 인터랙션이 필요하면 UIButtonComponent 추가
	//	auto& btn = mWorld->AddComponent<UIButtonComponent>(e);
	//	btn.mBaseSize = tr.mSize;
	//	btn.mOnClick = []() { gEngine->GetSceneManager().RequestScene(SceneId::Lobby); };

	mWorld->Initialize();


	mWorld->GetSystemManager()->RegisterSystem<TransformSystem>();
	mWorld->GetSystemManager()->RegisterSystem<AnimationSystem>();
	mWorld->GetSystemManager()->RegisterSystem<CameraSystem>();
	mWorld->GetSystemManager()->RegisterSystem<AudioVisualizerSystem>();
	mWorld->GetSystemManager()->RegisterSystem<UITransformSystem>();
	mWorld->GetSystemManager()->RegisterSystem<UIUpdateSystem>();
	mWorld->GetSystemManager()->RegisterSystem<UIButtonSystem>();
	mWorld->GetSystemManager()->RegisterSystem<AudioSystem>();
	auto* renderSystemMM = mWorld->GetSystemManager()->RegisterSystem<RenderSystem>();
	renderSystemMM->SetPipeline(make_shared<LobbyRenderPipeline>());
	mWorld->GetSystemManager()->RegisterSystem<UIRenderSystem>();

	mSceneId = SceneId::MainMenu;


}





void LobbyScene::Initialize()
{
	//PlayerPrefab player{mWorld.get()};
	mWorld->SetSceneId(mSceneId);
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

	for (int i = 0; i < 3; ++i) {
		Entity mEntityID = mWorld->CreateEntity();

		TransformComponent t{};
		shared_ptr<Mesh> phereMesh;
		shared_ptr<Material> material2;
		std::vector<shared_ptr<Material>> material2s;
		vector<shared_ptr<Animator>> anmators0;

		switch (i) {
		case 0:
			phereMesh = RESOURCEMANAGER.Get<Mesh>(L"SM_Rudwig_Body");
			material2 = RESOURCEMANAGER.Get<Material>(L"Anim_Rudwig_Attack_010");
			material2s.push_back(material2);
			material2 = RESOURCEMANAGER.Get<Material>(L"Anim_Rudwig_Attack_011");
			material2s.push_back(material2);
			/*material2 = RESOURCEMANAGER.Get<Material>(L"Anim_Rudwig_Idle0");
			material2s.push_back(material2);*/
			anmators0.push_back(RESOURCEMANAGER.Get<Animator>(L"Anim_Rudwig_Idle"));
			anmators0.push_back(RESOURCEMANAGER.Get<Animator>(L"Anim_Rudwig_Run"));
			mWorld->AddComponent<MannequinComponent>(mEntityID, i);
			break;
		case 1:
			phereMesh = RESOURCEMANAGER.Get<Mesh>(L"SM_Ibanix_Body");
			material2 = RESOURCEMANAGER.Get<Material>(L"Anim_Ibanix_Attack_010");
			material2s.push_back(material2);
			material2 = RESOURCEMANAGER.Get<Material>(L"Anim_Ibanix_Attack_011");
			material2s.push_back(material2);
			anmators0.push_back(RESOURCEMANAGER.Get<Animator>(L"Anim_Ibanix_Idle"));
			anmators0.push_back(RESOURCEMANAGER.Get<Animator>(L"Anim_Ibanix_Walk"));
			mWorld->AddComponent<MannequinComponent>(mEntityID, i);
			break;
		case 2:
			phereMesh = RESOURCEMANAGER.Get<Mesh>(L"SM_Fanthor_Body");
			material2 = RESOURCEMANAGER.Get<Material>(L"Anim_Fanthor_Attack_010");
			material2s.push_back(material2);
			material2 = RESOURCEMANAGER.Get<Material>(L"Anim_Fanthor_Attack_011");
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

	//{
	//	Entity text = mWorld->CreateEntity();
	//	auto& t = mWorld->AddComponent<UITextComponent>(text);
	//}


	{
		Entity Aim = mWorld->CreateEntity();
		shared_ptr<Material> scorem;
		scorem = RESOURCEMANAGER.Get<Material>(L"jAims");

		auto& t = mWorld->AddComponent<UITransformComponent>(Aim);
		t.mAnchor = Anchor::Center;
		t.mPosition = Vec2(-98.f, -64.f);
		t.mSize = Vec2(196.f, 128.f);

		auto& m = mWorld->AddComponent<UICusSpriteComponent>(Aim, scorem);
	}

	{
		Entity Ibanix_Ammo = mWorld->CreateEntity();
		shared_ptr<Material> scorem;
		scorem = RESOURCEMANAGER.Get<Material>(L"Ibanix_Ammo");

		auto& t = mWorld->AddComponent<UITransformComponent>(Ibanix_Ammo);
		t.mAnchor = Anchor::BottomLeft;
		t.mPosition = Vec2(212.f, -212.f);
		t.mSize = Vec2(196.f, 128.f);

		auto& m = mWorld->AddComponent<UICusSpriteComponent>(Ibanix_Ammo, scorem);
	}

	{
		Entity Fanthor_Portrait = mWorld->CreateEntity();

		shared_ptr<Material> scorem;
		scorem = RESOURCEMANAGER.Get<Material>(L"Fanthor_Portrait");

		auto& t = mWorld->AddComponent<UITransformComponent>(Fanthor_Portrait);
		t.mAnchor = Anchor::BottomLeft;
		t.mPosition = Vec2(32.f, -636.f);
		t.mSize = Vec2(196.f, 196.f);

		auto& m = mWorld->AddComponent<UICusSpriteComponent>(Fanthor_Portrait, scorem);
	}

	{
		Entity Ibanix_Portrait = mWorld->CreateEntity();

		shared_ptr<Material> scorem;
		scorem = RESOURCEMANAGER.Get<Material>(L"Ibanix_Portrait");

		auto& t = mWorld->AddComponent<UITransformComponent>(Ibanix_Portrait);
		t.mAnchor = Anchor::BottomLeft;
		t.mPosition = Vec2(32.f, -424.f);
		t.mSize = Vec2(196.f, 196.f);

		auto& m = mWorld->AddComponent<UICusSpriteComponent>(Ibanix_Portrait, scorem);
	}

	{
		Entity Rudwig_Portrait = mWorld->CreateEntity();

		shared_ptr<Material> scorem;
		scorem = RESOURCEMANAGER.Get<Material>(L"Rudwig_Portrait");

		auto& t = mWorld->AddComponent<UITransformComponent>(Rudwig_Portrait);
		t.mAnchor = Anchor::BottomLeft;
		t.mPosition = Vec2(32.f, -212.f);
		t.mSize = Vec2(196.f, 196.f);

		auto& m = mWorld->AddComponent<UICusSpriteComponent>(Rudwig_Portrait, scorem);
	}

#pragma endregion

	/////////////////////////////////////////////////////////////////////////



	mWorld->Initialize();

	// INPUT
	mWorld->GetSystemManager()->RegisterSystem<PlayerInputSystem>();

	// NETWORK
	mWorld->GetSystemManager()->RegisterSystem<NetRecvSystem>(mWorld->GetNetIdMap());
	mWorld->GetSystemManager()->RegisterSystem<NetSendSystem>();
	mWorld->GetSystemManager()->RegisterSystem<AnimationSystem>();
	mWorld->GetSystemManager()->RegisterSystem<CameraSystem>();
	mWorld->GetSystemManager()->RegisterSystem<EnemySystem>();
	mWorld->GetSystemManager()->RegisterSystem<TransformSystem>();
	mWorld->GetSystemManager()->RegisterSystem<MovementSystem>();
	mWorld->GetSystemManager()->RegisterSystem<AudioVisualizerSystem>();
	mWorld->GetSystemManager()->RegisterSystem<UITransformSystem>();
	mWorld->GetSystemManager()->RegisterSystem<UIUpdateSystem>();
	mWorld->GetSystemManager()->RegisterSystem<AudioSystem>();
	mWorld->GetSystemManager()->RegisterSystem<BeatSystem>();
	mWorld->GetSystemManager()->RegisterSystem<NetInterpolationSystem>();
	auto* renderSystemLB = mWorld->GetSystemManager()->RegisterSystem<RenderSystem>();
	renderSystemLB->SetPipeline(make_shared<LobbyRenderPipeline>());
	mWorld->GetSystemManager()->RegisterSystem<UIRenderSystem>();


	//{
	//	AUDIOMANAGER.InitSpectrumDSP(4096 * 4);

	//	Entity visEntity = mWorld->CreateEntity();
	//	AudioVisualizerComponent& vis = mWorld->AddComponent<AudioVisualizerComponent>(visEntity);

	//	// 선택: 위치/크기 커스터마이징
	//	vis.basePosition = Vec2(2560.f, 1240.f);  // 화면 하단 중앙
	//	vis.barWidth = 7.f;
	//	vis.barSpacing = 3.f;
	//	vis.maxBarHeight = 250.f;
	//	vis.gain = 8.f;
	//}

	{
		AUDIOMANAGER.InitSpectrumDSP(4096 * 4);

		Entity visEntity = mWorld->CreateEntity();
		AudioVisualizerComponent& vis = mWorld->AddComponent<AudioVisualizerComponent>(visEntity);

		// 선택: 위치/크기 커스터마이징
		vis.basePosition = Vec2(2560.f/2, 700.f);  // 화면 하단 중앙
		vis.barWidth = 6.f;
		vis.barSpacing = 0.5f;
		vis.maxBarHeight = 25.f;
		vis.gain = 8.f;

#ifdef _IMGUI

		IMGUIComponent& visImgui = mWorld->AddComponent<IMGUIComponent>(visEntity);
		std::vector<EditorProperty> props;
		props.push_back({ "Base Position",  PropertyType::Vec2,  &vis.basePosition,  0.f,    0.f });
		props.push_back({ "Bar Width",      PropertyType::Float, &vis.barWidth,       1.f,   50.f });
		props.push_back({ "Bar Spacing",    PropertyType::Float, &vis.barSpacing,     0.f,   20.f });
		props.push_back({ "Max Height",     PropertyType::Float, &vis.maxBarHeight,   10.f, 800.f });
		props.push_back({ "Gain",           PropertyType::Float, &vis.gain,           0.1f,  30.f });
		props.push_back({ "Rise Smooth",    PropertyType::Float, &vis.riseSmooth,     1.f,   50.f });
		props.push_back({ "Fall Smooth",    PropertyType::Float, &vis.fallSmooth,     0.1f,  20.f });
		props.push_back({ "Visible",        PropertyType::Bool,  &vis.isVisible,      0.f,    0.f });
		visImgui.RegisterEditorProperties(props);
		visImgui.SetName("Audio Visualizer");
#endif
	}
}

#pragma endregion

/// //////////////////////////////////////////////////////////////////////////////////
#pragma region Loading Scenes
void LoadingScene::Initialize()
{
	
	mWorld->SetSceneId(mSceneId);
	//PrefabFactory::RegisterAllPrefabs();
	TerrainPrefab terrain{ mWorld.get() };
	SkyBoxPrefab skybox{ mWorld.get() };
	DirLightPrefab light{ mWorld.get() };

	{
		Entity testCamera = mWorld->CreateEntity();
		TransformComponent t{};
		t.mLocalPosition = { 0.f, 0.f, -10.f };
		mWorld->AddComponent<MainCameraComponent>(testCamera);
		mWorld->AddComponent<CameraComponent>(testCamera);
		mWorld->AddComponent<TransformComponent>(testCamera, t);
	}

	{
		mLoadingImage = mWorld->CreateEntity();
		auto& tr = mWorld->AddComponent<UITransformComponent>(mLoadingImage);
		WindowInfo windowInfo = RENDERMANAGER.GetWindow();
		tr.mAnchor = Anchor::TopLeft;
		tr.mPosition = Vec2(0.f, 0.f);
		tr.mSize = Vec2(static_cast<float>(windowInfo.Width), static_cast<float>(windowInfo.Height));
		tr.mPivot = Vec2(0.f, 0.f);

		shared_ptr<Material> loadingMaterial = nullptr;
		//switch (gEngine->GetSceneManager().Get)
		//{
		//case SceneCommandType::LoadingThenScene:
		//	loadingMaterial = RESOURCEMANAGER.Get<Material>(L"Title_Background");
		//	break;
		//case SceneCommandType::LoadScene:
		//default:
		//	loadingMaterial = RESOURCEMANAGER.Get<Material>(L"Game_Loading_Background");
		//	break;
		//}
		loadingMaterial = RESOURCEMANAGER.Get<Material>(L"Game_Loading_Background");
		if (loadingMaterial == nullptr)
			loadingMaterial = RESOURCEMANAGER.Get<Material>(L"HPBAR");

		mWorld->AddComponent<UICusSpriteComponent>(mLoadingImage, loadingMaterial);
	}


	mWorld->Initialize();
}

void LoadingScene::Update(float deltaTime)
{
	UITextComponent* loadingText = mWorld->GetComponent<UITextComponent>(mLoadingText);
	if (loadingText)
	{
		loadingText->mText = gEngine->GetSceneManager().GetLoadingMessage();
		if (loadingText->mText.empty())
			loadingText->mText = L"Loading...";
	}

	mWorld->Update(deltaTime);
}
#pragma endregion
/// //////////////////////////////////////////////////////////////////////////////////



#pragma region Game Scenes
void FirstScene::Initialize()
{
	mWorld->SetSceneId(mSceneId);
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
	LoadJsonLevel(L"..\\Resources\\Json\\Map001_Export.json");
	//LoadCollisionJson(L"..\\Resources\\Json\\Map001_CRX.json");

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
		mWorld->AddComponent<RenderComponent>(enityt, mesh, materials);

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

		shared_ptr<Texture> texture = RESOURCEMANAGER.Get<Texture>(L"fire");
		Entity fire = mWorld->CreateEntity();
		auto& t = mWorld->AddComponent<UISpriteComponent>(fire, texture,
			Vec2(64.f, 64.f), 4, 1.f);
		auto& u = mWorld->AddComponent<UITransformComponent>(fire);
		u.mAnchor = Anchor::Center;
		u.mPosition = Vec2(0.f, 0.f);
		u.mSize = Vec2(64, 64);

	}


	{
		Entity Aim = mWorld->CreateEntity();
		shared_ptr<Material> scorem;
		scorem = RESOURCEMANAGER.Get<Material>(L"jAims");

		auto& t = mWorld->AddComponent<UITransformComponent>(Aim);
		t.mAnchor = Anchor::Center;
		t.mPosition = Vec2(-98.f, -64.f);
		t.mSize = Vec2(196.f, 128.f);

		auto& m = mWorld->AddComponent<UICusSpriteComponent>(Aim, scorem);
	}

	{
		Entity Ibanix_Ammo = mWorld->CreateEntity();
		shared_ptr<Material> scorem;
		scorem = RESOURCEMANAGER.Get<Material>(L"Ibanix_Ammo");

		auto& t = mWorld->AddComponent<UITransformComponent>(Ibanix_Ammo);
		t.mAnchor = Anchor::BottomLeft;
		t.mPosition = Vec2(212.f, -212.f);
		t.mSize = Vec2(196.f, 128.f);

		auto& m = mWorld->AddComponent<UICusSpriteComponent>(Ibanix_Ammo, scorem);
	}

	{
		Entity Fanthor_Portrait = mWorld->CreateEntity();

		shared_ptr<Material> scorem;
		scorem = RESOURCEMANAGER.Get<Material>(L"Fanthor_Portrait");

		auto& t = mWorld->AddComponent<UITransformComponent>(Fanthor_Portrait);
		t.mAnchor = Anchor::BottomLeft;
		t.mPosition = Vec2(32.f, -636.f);
		t.mSize = Vec2(196.f, 196.f);
		t.mPivot = Vec2(0.5f, 0.5f);

		auto& m = mWorld->AddComponent<UIActionComponent>(Fanthor_Portrait);
		m.mDuration = 0.5f;
		m.mActor = UIActor::Player;
		m.mState = UIActionState::Bounce;
		m.mIsLoop = true;
		m.mBounceAmplitude =20.f;
		m.mBounceFrequency = 2.f;
		m.mBounceDamping = 10.f;
		mWorld->AddComponent<UICusSpriteComponent>(Fanthor_Portrait, scorem);

#ifdef _IMGUI

		std::vector<EditorProperty> props;
		props.push_back({ "Fanthor_Portrait Position1",  PropertyType::Vec2,  &(t.mPosition),  0.f,    0.f });
		props.push_back({ "Fanthor_Portrait Bounce",  PropertyType::Float,  &(m.mBounceDamping),  -10.f, 10.f});
		IMGUIComponent& visImgui = mWorld->AddComponent<IMGUIComponent>(Fanthor_Portrait);
		visImgui.RegisterEditorProperties(props);
		visImgui.SetName("Menu");
#endif

	}

	/*{
		Entity Ibanix_Portrait = mWorld->CreateEntity();

		shared_ptr<Material> scorem;
		scorem = RESOURCEMANAGER.Get<Material>(L"Ibanix_Portrait");

		auto& t = mWorld->AddComponent<UITransformComponent>(Ibanix_Portrait);
		t.mAnchor = Anchor::BottomLeft;
		t.mPosition = Vec2(32.f, -424.f);
		t.mSize = Vec2(196.f, 196.f);

		auto& m = mWorld->AddComponent<UIActionComponent>(Ibanix_Portrait);
		m.mDuration = 30.f;
		m.mActor = UIActor::Player;
		m.mState = UIActionState::Vibration;
		mWorld->AddComponent<UICusSpriteComponent>(Ibanix_Portrait, scorem);
	}
*/

	





#pragma endregion

	/////////////////////////////////////////////////////////////////////////



	mWorld->Initialize();

	// INPUT
	mWorld->GetSystemManager()->RegisterSystem<PlayerInputSystem>();

	// NETWORK
	mWorld->GetSystemManager()->RegisterSystem<NetRecvSystem>(mWorld->GetNetIdMap());
	mWorld->GetSystemManager()->RegisterSystem<NetSendSystem>();
	mWorld->GetSystemManager()->RegisterSystem<AnimationSystem>();
	mWorld->GetSystemManager()->RegisterSystem<CameraSystem>();
	
	mWorld->GetSystemManager()->RegisterSystem<EnemySystem>();
	mWorld->GetSystemManager()->RegisterSystem<PlayerSystem>();
	mWorld->GetSystemManager()->RegisterSystem<TransformSystem>();
	mWorld->GetSystemManager()->RegisterSystem<MovementSystem>();
	mWorld->GetSystemManager()->RegisterSystem<AudioVisualizerSystem>();
	mWorld->GetSystemManager()->RegisterSystem<UITransformSystem>();
	mWorld->GetSystemManager()->RegisterSystem<UIUpdateSystem>();
	mWorld->GetSystemManager()->RegisterSystem<AudioSystem>();
	mWorld->GetSystemManager()->RegisterSystem<BeatSystem>();
	mWorld->GetSystemManager()->RegisterSystem<NetInterpolationSystem>();
	auto* renderSystemFS = mWorld->GetSystemManager()->RegisterSystem<RenderSystem>();
	renderSystemFS->SetPipeline(make_shared<GameRenderPipeline>());
	mWorld->GetSystemManager()->RegisterSystem<UIRenderSystem>();


}

void SecondScene::Initialize()
{
	mWorld->SetSceneId(mSceneId);
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
	LoadJsonLevel(L"..\\Resources\\Json\\Map001_Export.json");
	//LoadCollisionJson(L"..\\Resources\\Json\\Map001_CRX.json");

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
		mWorld->AddComponent<RenderComponent>(enityt, mesh, materials);

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

		shared_ptr<Texture> texture = RESOURCEMANAGER.Get<Texture>(L"fire");
		Entity fire = mWorld->CreateEntity();
		auto& t = mWorld->AddComponent<UISpriteComponent>(fire, texture,
			Vec2(64.f, 64.f), 4, 1.f);
		auto& u = mWorld->AddComponent<UITransformComponent>(fire);
		u.mAnchor = Anchor::Center;
		u.mPosition = Vec2(0.f, 0.f);
		u.mSize = Vec2(64, 64);

	}

	{
		Entity text = mWorld->CreateEntity();
		auto& t = mWorld->AddComponent<UITextComponent>(text);
		t.mText = L"IN GAME";
	}

	{
		Entity Aim = mWorld->CreateEntity();
		shared_ptr<Material> scorem;
		scorem = RESOURCEMANAGER.Get<Material>(L"jAims");

		auto& t = mWorld->AddComponent<UITransformComponent>(Aim);
		t.mAnchor = Anchor::Center;
		t.mPosition = Vec2(-98.f, -64.f);
		t.mSize = Vec2(196.f, 128.f);

		auto& m = mWorld->AddComponent<UICusSpriteComponent>(Aim, scorem);
	}

	{
		Entity Ibanix_Ammo = mWorld->CreateEntity();
		shared_ptr<Material> scorem;
		scorem = RESOURCEMANAGER.Get<Material>(L"Ibanix_Ammo");

		auto& t = mWorld->AddComponent<UITransformComponent>(Ibanix_Ammo);
		t.mAnchor = Anchor::BottomLeft;
		t.mPosition = Vec2(212.f, -212.f);
		t.mSize = Vec2(196.f, 128.f);

		auto& m = mWorld->AddComponent<UICusSpriteComponent>(Ibanix_Ammo, scorem);
	}

	{
		Entity Fanthor_Portrait = mWorld->CreateEntity();

		shared_ptr<Material> scorem;
		scorem = RESOURCEMANAGER.Get<Material>(L"Fanthor_Portrait");

		auto& t = mWorld->AddComponent<UITransformComponent>(Fanthor_Portrait);
		t.mAnchor = Anchor::BottomLeft;
		t.mPosition = Vec2(32.f, -636.f);
		t.mSize = Vec2(196.f, 196.f);

		auto& m = mWorld->AddComponent<UICusSpriteComponent>(Fanthor_Portrait, scorem);
	}

	{
		Entity Ibanix_Portrait = mWorld->CreateEntity();

		shared_ptr<Material> scorem;
		scorem = RESOURCEMANAGER.Get<Material>(L"Ibanix_Portrait");

		auto& t = mWorld->AddComponent<UITransformComponent>(Ibanix_Portrait);
		t.mAnchor = Anchor::BottomLeft;
		t.mPosition = Vec2(32.f, -424.f);
		t.mSize = Vec2(196.f, 196.f);

		auto& m = mWorld->AddComponent<UICusSpriteComponent>(Ibanix_Portrait, scorem);
	}

	{
		Entity Rudwig_Portrait = mWorld->CreateEntity();

		shared_ptr<Material> scorem;
		scorem = RESOURCEMANAGER.Get<Material>(L"Rudwig_Portrait");

		auto& t = mWorld->AddComponent<UITransformComponent>(Rudwig_Portrait);
		t.mAnchor = Anchor::BottomLeft;
		t.mPosition = Vec2(32.f, -212.f);
		t.mSize = Vec2(196.f, 196.f);

		auto& m = mWorld->AddComponent<UICusSpriteComponent>(Rudwig_Portrait, scorem);
	}

#pragma endregion

	/////////////////////////////////////////////////////////////////////////



	mWorld->Initialize();

	//// INPUT
	//mWorld->GetSystemManager()->RegisterSystem<PlayerInputSystem>();

	//// NETWORK
	//mWorld->GetSystemManager()->RegisterSystem<NetRecvSystem>(mWorld->GetNetIdMap());
	//mWorld->GetSystemManager()->RegisterSystem<NetSendSystem>();
	//mWorld->GetSystemManager()->RegisterSystem<AnimationSystem>();
	//mWorld->GetSystemManager()->RegisterSystem<CameraSystem>();
	//mWorld->GetSystemManager()->RegisterSystem<EnemySystem>();
	//mWorld->GetSystemManager()->RegisterSystem<TransformSystem>();
	//mWorld->GetSystemManager()->RegisterSystem<MovementSystem>();
	//mWorld->GetSystemManager()->RegisterSystem<AudioVisualizerSystem>();
	//mWorld->GetSystemManager()->RegisterSystem<UITransformSystem>();
	//mWorld->GetSystemManager()->RegisterSystem<UIUpdateSystem>();
	//mWorld->GetSystemManager()->RegisterSystem<AudioSystem>();
	//mWorld->GetSystemManager()->RegisterSystem<BeatSystem>();
	//mWorld->GetSystemManager()->RegisterSystem<NetInterpolationSystem>();
	//mWorld->GetSystemManager()->RegisterSystem<RenderSystem>();
	//mWorld->GetSystemManager()->RegisterSystem<UIRenderSystem>();



}
#pragma endregion

#pragma region Victory / Lose Scene

void VictoryScene::Initialize()
{
	mWorld->Initialize();
}

void LoseScene::Initialize()
{
	mWorld->Initialize();

}

#pragma endregion