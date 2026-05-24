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
#include "ParticleComponent.h"
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
#include "LobbyRoomStateComponent.h"
#include "LobbyRoomUIFeature.h"

#include "RenderSystem.h"
#include "GameRenderPipeline.h"
#include "LobbyRenderPipeline.h"
#include "CameraSystem.h"
#include "AudioSystem.h"
#include "TransformSystem.h"
#include "AnimationSystem.h"
#include "CpuAnimationSystem.h"
#include "PlayerSystem.h"
#include "ParticleSystem.h"
#include "VfxSystem.h"

#include "UIRenderSystem.h"
#include "UIUpdateSystem.h"
#include "IMGUISystem.h"
#include "BeatSystem.h"
#include "MovementSystem.h"
#include "GamePhaseSystem.h"
#include "DamageFeedbackSystem.h"
#include "DamagePopupUpdateFeature.h"

#include "NetRecvSystem.h"
#include "GameRuleComponent.h"
#include "PlayerInputSystem.h"
#include "EnemySystem.h"
#include "CollisionSystem.h"
#include "NetInterpolationSystem.h"
#include "AudioVisualizerSystem.h"
#include "InputManager.h"
#include "GameMode.h"
#include "UIButtonComponent.h"
#include "UIButtonSystem.h"
#include "UIFeature.h"
#include "UIActionUpdateFeature.h"
#include "UIAudioVisualizerFeature.h"
#include "UICommonUpdateFeature.h"
#include "UIHpBarUpdateFeature.h"
#include "UIGameInfoUpdateFeature.h"
#include "UIPhaseProgressUpdateFeature.h"

#include "MainMenuController.h"
#include "MainMenuCameraComponent.h"
#include "MainMenuSystem.h"
#include "MainMenuCameraSystem.h"
#include "UIButtonFactory.h"



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



void Scene::LoadJsonLevelFBX(const wstring& path) {
	LevelImportData level = RESOURCEMANAGER.LoadMapResourceJson(path);

	std::unordered_set<std::string> uniqueFbxNames;

	for (const auto& inst : level.instances)
	{
		if (inst.fbx.empty())
			continue;

		std::string name = filesystem::path(inst.fbx).filename().stem().string();
		if (name.empty())
			continue;

		uniqueFbxNames.insert(name);
	}

	for (const auto& fbxName : uniqueFbxNames)
	{
		RESOURCEMANAGER.LoadFBXMesh(s2ws("..\\Resources\\Map\\" + fbxName + ".fbx"));
	}

}

void Scene::LoadJsonLevelData(const wstring& path) {
	int i = 0;
	try
	{
		LevelImportData level = RESOURCEMANAGER.LoadMapResourceJson(path);

		for (const auto& inst : level.instances)
		{
			// 파일명만 추출
			std::string name = filesystem::path(inst.fbx).filename().stem().string();
			/*name = "..\\Resources\\Map\\" + name + ".fbx";*/
			shared_ptr<FBXData> data = RESOURCEMANAGER.Get<FBXData>(s2ws(name));

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
			//render.mCheckFrustum = false;
			render.SetMesh(data->GetMeshs().at(0));
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



void Scene::LoadJsonLevel(const wstring& path)
{

	int i = 0;
	try
	{
		LevelImportData level = RESOURCEMANAGER.LoadMapResourceJson(path);

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
			render.SetMesh(data->GetMeshs().at(0));
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
	int loadedInstanceCount = 0;
	int loadedMeshCount = 0;
	int skippedCount = 0;
	try
	{
		LevelImportData level = RESOURCEMANAGER.LoadMapResourceJson(path);
		auto physicsWorld = mWorld->GetPhysicsWorld();
		if (!physicsWorld)
			throw std::runtime_error("LoadCollisionJson requires PhysicsWorld");

		for (const auto& inst : level.instances)
		{
			if (inst.fbx.empty())
			{
				++skippedCount;
				continue;
			}

			std::string stem = filesystem::path(inst.fbx).filename().stem().string();
			if (stem.empty())
			{
				++skippedCount;
				continue;
			}

			std::wstring fbxPath = L"..\\Resources\\Map\\" + s2ws(stem) + L".fbx";
			shared_ptr<FBXData> collisionFbx = RESOURCEMANAGER.LoadFBXMesh(fbxPath);
			if (!collisionFbx || collisionFbx->GetColliders().empty())
			{
				++skippedCount;
				continue;
			}

			Entity entity = mWorld->CreateEntity();
			TransformComponent transform{};
			transform.mWorldMatrix = inst.worldMtx;
			TransformComponent& trans = mWorld->AddComponent<TransformComponent>(entity, transform);
			trans.mIsStatic = true;

			bool registeredAnyMesh = false;
			for (const shared_ptr<CollisionMesh>& colliderMesh : collisionFbx->GetColliders())
			{
				if (!colliderMesh)
					continue;
				if (physicsWorld->AddStaticCollisionMesh(entity, *colliderMesh, inst.worldMtx))
				{
					registeredAnyMesh = true;
					++loadedMeshCount;
				}
			}

			if (registeredAnyMesh)
				++loadedInstanceCount;
			else
				++skippedCount;
		}

		if (loadedMeshCount > 0)
			physicsWorld->OptimizeJoltStaticCollision();

		std::cout << "[Jolt] collision instances=" << loadedInstanceCount
			<< " meshes=" << loadedMeshCount
			<< " skipped=" << skippedCount << std::endl;
	}
	catch (const std::exception& e)
	{
		std::cerr << "LoadCollisionJson failed: " << e.what() << "\n";
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

#pragma region Loading Scenes
void LoadingScene::Initialize()
{
	PrefabFactory::RegisterAllPrefabs();
	// SkyBoxPrefab skybox{ mWorld.get() };
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
		Entity mLoadingImage = mWorld->CreateEntity();
		auto& tr = mWorld->AddComponent<UITransformComponent>(mLoadingImage);
		WindowInfo windowInfo = RENDERMANAGER.GetWindow();
		tr.mAnchor = Anchor::TopLeft;
		tr.mPosition = Vec2(0.f, 0.f);
		tr.mSize = Vec2(static_cast<float>(windowInfo.Width), static_cast<float>(windowInfo.Height));
		tr.mPivot = Vec2(0.f, 0.f);


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
		shared_ptr<Texture> loadingMaterial = RESOURCEMANAGER.Get<Texture>(L"UI_Loading_Main_01");


		mWorld->AddComponent<UISpriteComponent>(mLoadingImage, loadingMaterial);
	}

	{

		shared_ptr<Texture> texture = RESOURCEMANAGER.Get<Texture>(L"UI_Loading_Circle");
		Entity Loading_Circle = mWorld->CreateEntity();
		auto& t = mWorld->AddComponent<UISpriteComponent>(Loading_Circle, texture,
			Vec2(1024.f, 1024.f), 5, 1.f);
		auto& u = mWorld->AddComponent<UITransformComponent>(Loading_Circle);
		u.mAnchor = Anchor::TopLeft;
		u.mPosition = Vec2(-256.f, -256.f);
		u.mSize = Vec2(1440, 1440);

	}

	{

		shared_ptr<Texture> texture = RESOURCEMANAGER.Get<Texture>(L"UI_Loading_Circle");
		Entity Loading_Circle = mWorld->CreateEntity();
		auto& t = mWorld->AddComponent<UISpriteComponent>(Loading_Circle, texture,
			Vec2(1024.f, 1024.f), 5, 0.5f);
		auto& u = mWorld->AddComponent<UITransformComponent>(Loading_Circle);
		u.mAnchor = Anchor::BottomRight;
		u.mPosition = Vec2(-768.f, -512.f);
		u.mSize = Vec2(1536, 1536);

	}

	{
		mProgressBar = mWorld->CreateEntity();
		auto& tr = mWorld->AddComponent<UITransformComponent>(mProgressBar);
		tr.mAnchor = Anchor::BottomLeft;
		tr.mPosition = Vec2(50.f, -50.f);
		tr.mSize = Vec2(0.f, 30.f); // 초기 크기 (진행률에 따라 변경)
		tr.mPivot = Vec2(0.f, 0.5f);
		mProgressBarMaxWidth = 400.f; // 최대 너비 설정
		shared_ptr<Texture> progressBarMaterial = RESOURCEMANAGER.Get<Texture>(L"HPBAR");
		mWorld->AddComponent<UISpriteComponent>(mProgressBar, progressBarMaterial);
	}



	mWorld->Initialize();
	mWorld->GetSystemManager()->RegisterSystem<TransformSystem>();
#if USE_CPU_ANIMATION
	mWorld->GetSystemManager()->RegisterSystem<CpuAnimationSystem>();
#else
	mWorld->GetSystemManager()->RegisterSystem<AnimationSystem>();
#endif
	mWorld->GetSystemManager()->RegisterSystem<CameraSystem>();
	//mWorld->GetSystemManager()->RegisterSystem<AudioVisualizerSystem>();
	mWorld->GetSystemManager()->RegisterSystem<UITransformSystem>();
	auto* uiUpdateSystem = mWorld->GetSystemManager()->RegisterSystem<UIUpdateSystem>();
	uiUpdateSystem->SetFeatures(&mUIFeatures);
	mWorld->GetSystemManager()->RegisterSystem<UIButtonSystem>();
	mWorld->GetSystemManager()->RegisterSystem<AudioSystem>();
	auto* renderSystemMM = mWorld->GetSystemManager()->RegisterSystem<RenderSystem>();
	renderSystemMM->SetPipeline(make_shared<LobbyRenderPipeline>());
	auto* uiRenderSystem = mWorld->GetSystemManager()->RegisterSystem<UIRenderSystem>();
	uiRenderSystem->SetFeatures(&mUIFeatures);
}

void LoadingScene::Shudown()
{
	mWorld->Shutdown();
}

void LoadingScene::Render()
{
	mWorld->Render();
}


bool LoadingScene::LoadScene(SceneId id)
{
	mTargetSceneId = id;

	std::wstring mapPath;
	switch ((uint8)id) {
	case (uint8)SceneId::FirstGame: {
		mapPath = L"..\\Resources\\Json\\Map001_Export.json";
	}
								  break;
	case (uint8)SceneId::SecondGame: {
		mapPath = L"..\\Resources\\Json\\Map001_Export.json";
	}
								   break;
	default:
		return false;
		break;
	}

	LevelImportData level = RESOURCEMANAGER.LoadMapResourceJson(mapPath);

	std::unordered_set<std::string> uniqueFbxNames;

	for (const auto& inst : level.instances)
	{
		if (inst.fbx.empty())
			continue;

		std::string name = filesystem::path(inst.fbx).filename().stem().string();
		if (name.empty())
			continue;

		uniqueFbxNames.insert(name);
	}

	for (const auto& fbxName : uniqueFbxNames)
	{
		mLoadTasks.push([fbxName]() {
			std::string name = "..\\Resources\\Map\\" + fbxName + ".fbx";
			shared_ptr<FBXData> data = RESOURCEMANAGER.LoadFBXMesh(s2ws(name));
			});
	}

	mTotalTaskCount = (int32)mLoadTasks.size();

	return true;
}

void LoadingScene::ProcessTask()
{
	if (!mLoadTasks.empty())
	{
		auto task = mLoadTasks.front();
		task(); // 작업 실행
		mLoadTasks.pop();
	}
}


void LoadingScene::Update(float deltaTime)
{
	// 진행률로 프로그레스바 UI 크기 갱신
	float progress = GetProgress();
	UITransformComponent* bar = mWorld->GetComponent<UITransformComponent>(mProgressBar);
	if (bar)
		bar->mSize.x = mProgressBarMaxWidth * progress;



	/*UITextComponent* loadingText = mWorld->GetComponent<UITextComponent>("AA");
	if (loadingText)
	{
		loadingText->mText = gEngine->GetSceneManager().GetLoadingMessage();
		if (loadingText->mText.empty())
			loadingText->mText = L"Loading...";
	}*/

	mWorld->Update(deltaTime);
}

#pragma endregion

//////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////

#pragma region Menu Scenes

void MainMenuScene::Initialize()
{
	mWorld->SetSceneId(mSceneId);
	//PlayerPrefab player{mWorld.get()};
	PrefabFactory::RegisterAllPrefabs();
	//TerrainPrefab terrain{ mWorld.get() };
	SkyBoxPrefab skybox{ mWorld.get() };
	DirLightPrefab light{ mWorld.get() };

	// [참고] 현재 FBX LOADER에서 NormalMap을 읽지 못하게 함.
	// 
		//LoadJsonLevel(L"..\\Resources\\Json\\M_StylizedStudyLogCabin_A1_Export.json");
		// LoadJsonLevel(L"..\\Resources\\Json\\ThirdPersonMap_Export.json");
	LoadJsonLevelFBX(L"..\\Resources\\Json\\Map_Title_Export.json");
	LoadJsonLevelData(L"..\\Resources\\Json\\Map_Title_Export.json");
	//LoadCollisionJson(L"..\\Resources\\Json\\Map001_CRX.json");

	{ // Title LOGO
		Entity logo = mWorld->CreateEntity();
		shared_ptr<Mesh> data = RESOURCEMANAGER.Get<Mesh>(L"Rectangle");
		std::vector<std::shared_ptr<Material>> materials = { RESOURCEMANAGER.Get<Material>(L"UI_Logo") };

		RenderComponent& render = mWorld->AddComponent<RenderComponent>(logo);
		render.mMaterials = materials;
		render.mCheckFrustum = false;
		render.SetMesh(data);

		TransformComponent& transform = mWorld->AddComponent<TransformComponent>(logo);
		transform.mLocalPosition = { 274.f, 648.f, -4270.f };
		transform.mLocalRotationE = { 0.f, 180.0f, 0.0f };
		transform.mLocalScale = { 300.f, 300.f, 1.f };

		IMGUIComponent& imgui = mWorld->AddComponent<IMGUIComponent>(logo);
		std::vector< EditorProperty> props;
		props.push_back({ "Position", PropertyType::Vec3, &(transform.mLocalPosition), 0.f, 0.f });
		props.push_back({ "Rot", PropertyType::Vec3, &(transform.mLocalRotationE), 0.f, 0.f });
		imgui.RegisterEditorProperties(props);
		imgui.SetName("TITLE");




	}

	/////////////////////////////////////////////////////////////////////////
	Entity mainMenuCamera = mWorld->CreateEntity();

	// 메인 메뉴 카메라
	{
		TransformComponent t{};
		t.mLocalPosition = { 370.f,  490.f, -4058.f };	// 기본 값
		t.mLocalRotationE = { -18.f, -142.0f, 0.f };
		mWorld->AddComponent<MainCameraComponent>(mainMenuCamera);
		mWorld->AddComponent<CameraComponent>(mainMenuCamera);
		mWorld->AddComponent<TransformComponent>(mainMenuCamera, t);

		// 메인메뉴 카메라 6 view 로드
		auto& mm = mWorld->AddComponent<MainMenuCameraComponent>(mainMenuCamera);
		std::vector<CameraView> views = RESOURCEMANAGER.LoadCameraViews(
			L"..\\Resources\\Json\\MainMenuCameraViews.json");

		mm.mLoaded = (views.size() == (size_t)MainMenuView::Count);
		if (mm.mLoaded)
		{
			std::copy_n(views.begin(), (size_t)MainMenuView::Count, mm.mViews.begin());

			const auto& v0 = mm.View(MainMenuView::Title);
			TransformComponent* trC = mWorld->GetComponent<TransformComponent>(mainMenuCamera);
			trC->mLocalPosition = v0.position;
			Vec3 eR = v0.rotation.ToEuler();
			trC->mLocalRotationE = Vec3(
				XMConvertToDegrees(eR.x),
				XMConvertToDegrees(eR.y),
				XMConvertToDegrees(eR.z));


			// JSON fov 는 UE 가로 화각(도)에서 종횡비 기반 세로 화각(라디안)으로 변환
			if (CameraComponent* ccC = mWorld->GetComponent<CameraComponent>(mainMenuCamera))
			{
				const float aspect  = ccC->mWidth / ccC->mHeight;
				const float hFovRad = XMConvertToRadians(v0.fovDeg);
				ccC->SetFOV(2.f * atanf(tanf(hFovRad * 0.5f) / aspect));
			}

			mm.mCurrent = MainMenuView::Title;
			mm.mTarget = MainMenuView::Title;
			mm.mBlendT = 1.f;
		}

		// 메인메뉴 상태 머신 컨트롤러
		mWorld->AddComponent<MainMenuController>(mainMenuCamera);
	}

	/////////////////////////////////////////////////////////////////////////
	// 메인메뉴 버튼 UI
	/////////////////////////////////////////////////////////////////////////
	{
		const Vec2  btnSize = { 320.f, 72.f };
		const float startX = 850.f;   // 화면 중앙 기준 X 오프셋 (위쪽 버튼)
		const float startY = 270.f;   // 화면 중앙 기준 Y 오프셋 (위쪽 버튼)
		const float gap = 100.f;

		// (비주얼 종류: UIButtonVisual::Texture / Vfx / SpriteSheet)

		// 상태 전환 헬퍼
		Entity ctrlEnt = mainMenuCamera;
		auto requestState = [this, ctrlEnt](MainMenuState s)
			{
				if (auto* c = mWorld->GetComponent<MainMenuController>(ctrlEnt))
					c->Request(s);
			};

		// 메인 메뉴 버튼
		Entity bGameStart = CreateUIButton(mWorld.get(), {
			.position = Vec2(startX, startY + gap * 0),
			.size = btnSize,
			.visual = UIButtonVisual::Vfx,
			.resKey = L"VFX_UI_Select",
			.label = L"GAMESTART",
			.onClick = [this, requestState]()
			{
				requestState(MainMenuState::RoomList);
				if (Network::GetInstance().Awake()) {
					mGameMode->mTargetSceneId = SceneId::Lobby;
					mGameMode->IsSceneChanging() = true;
				}
			},
			});
		Entity bManual = CreateUIButton(mWorld.get(), {
			.position = Vec2(startX, startY + gap * 1),
			.size = btnSize,
			.visual = UIButtonVisual::Vfx,
			.resKey = L"VFX_UI_Select",
			.label = L"MANUAL",
			.onClick = [requestState]() { requestState(MainMenuState::Manual); },
			});
		Entity bSetting = CreateUIButton(mWorld.get(), {
			.position = Vec2(startX, startY + gap * 2),
			.size = btnSize,
			.visual = UIButtonVisual::Vfx,
			.resKey = L"VFX_UI_Select",
			.label = L"SETTING",
			.onClick = [requestState]() { requestState(MainMenuState::Setting); },
			});
		Entity bMainExit = CreateUIButton(mWorld.get(), {
			.position = Vec2(startX, startY + gap * 3),
			.size = btnSize,
			.visual = UIButtonVisual::Vfx,
			.resKey = L"VFX_UI_Select",
			.label = L"EXIT",
			.onClick = [requestState]() { requestState(MainMenuState::Exit); },
			});

		// 서브 화면(Setting/Manual/RoomList) 공유 Back/Exit
		Entity bBack = CreateUIButton(mWorld.get(), {
			.position = Vec2(startX, startY + gap * 4),
			.size = btnSize,
			.visual = UIButtonVisual::Vfx,
			.resKey = L"VFX_UI_Select",
			.label = L"BACK",
			.onClick = [requestState]() { requestState(MainMenuState::MainMenu); },
			});
		Entity bSubExit = CreateUIButton(mWorld.get(), {
			.position = Vec2(startX, startY + gap * 5),
			.size = btnSize,
			.visual = UIButtonVisual::Vfx,
			.resKey = L"VFX_UI_Select",
			.label = L"EXIT",
			.onClick = [requestState]() { requestState(MainMenuState::Exit); },
			});

		// Exit 확인 Yes/No
		Entity bYes = CreateUIButton(mWorld.get(), {
			.position = Vec2(-200.f, 0.f),
			.size = btnSize,
			.visual = UIButtonVisual::Vfx,
			.resKey = L"VFX_UI_Select",
			.label = L"YES",
			.onClick = []() { PostQuitMessage(0); },
			});
		Entity bNo = CreateUIButton(mWorld.get(), {
			.position = Vec2(200.f, 0.f),
			.size = btnSize,
			.visual = UIButtonVisual::Vfx,
			.resKey = L"VFX_UI_Select",
			.label = L"NO",
			.onClick = [requestState]() { requestState(MainMenuState::MainMenu); },
			});

		// Title 텍스트 (PRESS ANY KEY)
		Entity titleHint = mWorld->CreateEntity();
		{
			auto& tr = mWorld->AddComponent<UITransformComponent>(titleHint);
			tr.mAnchor = Anchor::Center;
			tr.mPosition = Vec2(0.f, 200.f);
			tr.mSize = Vec2(400.f, 80.f);
			tr.mPivot = Vec2(0.5f, 0.5f);
			tr.mUILayerIndex = 5;
			auto& txt = mWorld->AddComponent<UITextComponent>(titleHint);
			txt.mText = L"PRESS ANY KEY";
		}

		// 상태별 entity 등록, Title 만 visible
		auto* ctrl = mWorld->GetComponent<MainMenuController>(mainMenuCamera);
		ctrl->mStateEntities[(size_t)MainMenuState::Title] = { titleHint };
		ctrl->mStateEntities[(size_t)MainMenuState::MainMenu] = { bGameStart, bManual, bSetting, bMainExit };
		ctrl->mStateEntities[(size_t)MainMenuState::Setting] = { bBack, bSubExit };
		ctrl->mStateEntities[(size_t)MainMenuState::Manual] = { bBack, bSubExit };
		ctrl->mStateEntities[(size_t)MainMenuState::RoomList] = { bBack, bSubExit };
		ctrl->mStateEntities[(size_t)MainMenuState::Exit] = { bYes, bNo };

		auto applyVisible = [this](Entity e, bool v)
			{
				if (auto* sp = mWorld->GetComponent<UISpriteComponent>(e)) sp->mVisible = v;
				if (auto* vf = mWorld->GetComponent<UIVfxComponent>(e))    vf->mVisible = v;
				if (auto* tx = mWorld->GetComponent<UITextComponent>(e))   tx->mVisible = v;
				if (auto* bt = mWorld->GetComponent<UIButtonComponent>(e)) bt->mEnabled = v;
			};
		for (int s = 0; s < (int)MainMenuState::Count; ++s)
		{
			const bool show = (s == (int)MainMenuState::Title);
			for (Entity e : ctrl->mStateEntities[s])
				applyVisible(e, show);
		}

#ifdef _IMGUI
		UITransformComponent* vis = mWorld->GetComponent<UITransformComponent>(bGameStart);
		std::vector<EditorProperty> props;
		props.push_back({ "GameStart Pos", PropertyType::Vec2, &(vis->mPosition), 0.f, 0.f });
		vis = mWorld->GetComponent<UITransformComponent>(bManual);
		props.push_back({ "Manual Pos",    PropertyType::Vec2, &(vis->mPosition), 0.f, 0.f });
		vis = mWorld->GetComponent<UITransformComponent>(bSetting);
		props.push_back({ "Setting Pos",   PropertyType::Vec2, &(vis->mPosition), 0.f, 0.f });
		vis = mWorld->GetComponent<UITransformComponent>(bMainExit);
		props.push_back({ "Exit Pos",      PropertyType::Vec2, &(vis->mPosition), 0.f, 0.f });
		IMGUIComponent& visImgui = mWorld->AddComponent<IMGUIComponent>(bGameStart);
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
			material2 = RESOURCEMANAGER.Get<Material>(L"Anim_Rudwig_Base0")->Clone();
			RESOURCEMANAGER.Add<Material>(L"Anim_Rudwig_Base0S", material2);
			material2 = RESOURCEMANAGER.Get<Material>(L"Anim_Rudwig_Base0S");
			material2->SetShader(L"Solid");
			material2->GetParams().ExtValue[0] = Vec4(0.9f, 0.8f, 0.1f, 1.f); // RimPower
			material2s.push_back(material2);


			material2 = RESOURCEMANAGER.Get<Material>(L"Anim_Rudwig_Base1")->Clone();
			RESOURCEMANAGER.Add<Material>(L"Anim_Rudwig_Base1S", material2);
			material2 = RESOURCEMANAGER.Get<Material>(L"Anim_Rudwig_Base1S");
			material2->SetShader(L"Solid");
			material2->GetParams().ExtValue[0] = Vec4(0.8f, 0.7f, 0.0f, 1.f); // RimPower
			material2s.push_back(material2);

			anmators0.push_back(RESOURCEMANAGER.Get<Animator>(L"Anim_Rudwig_Idle"));
			anmators0.push_back(RESOURCEMANAGER.Get<Animator>(L"Anim_Rudwig_Run"));
			//mWorld->AddComponent<MannequinComponent>(mEntityID, i);
			t.mLocalPosition = { 139.0f, 454.0f,-4461.0f };
			break;
		case 1:
			phereMesh = RESOURCEMANAGER.Get<Mesh>(L"SM_Ibanix_Body");
			material2 = RESOURCEMANAGER.Get<Material>(L"Anim_Ibanix_Base0")->Clone();
			RESOURCEMANAGER.Add<Material>(L"Anim_Ibanix_Base0S", material2);
			material2 = RESOURCEMANAGER.Get<Material>(L"Anim_Ibanix_Base0S");
			material2->SetShader(L"Solid");
			material2->GetParams().ExtValue[0] = Vec4(0.2f, 0.6f, 0.2f, 1.f); // RimPower
			material2s.push_back(material2);



			material2 = RESOURCEMANAGER.Get<Material>(L"Anim_Ibanix_Attack_011")->Clone();
			RESOURCEMANAGER.Add<Material>(L"Anim_Ibanix_Attack_011S", material2);
			material2 = RESOURCEMANAGER.Get<Material>(L"Anim_Ibanix_Attack_011S");
			material2->SetShader(L"Solid");
			material2->GetParams().ExtValue[0] = Vec4(0.1f, 0.5f, 0.1f, 1.f); // RimPower
			material2s.push_back(material2);
			anmators0.push_back(RESOURCEMANAGER.Get<Animator>(L"Anim_Ibanix_Idle"));
			anmators0.push_back(RESOURCEMANAGER.Get<Animator>(L"Anim_Ibanix_Walk"));
			//mWorld->AddComponent<MannequinComponent>(mEntityID, i);
			t.mLocalPosition = { 71.0f, 454.0f, -4136.0f };
			t.mLocalRotationE = { 0.f, 90.f, 0.f };
			break;
		case 2:
			phereMesh = RESOURCEMANAGER.Get<Mesh>(L"SM_Fanthor_Body");
			material2 = RESOURCEMANAGER.Get<Material>(L"Anim_Fanthor_Attack_010")->Clone();
			RESOURCEMANAGER.Add<Material>(L"Anim_Fanthor_Attack_010S", material2);
			material2 = RESOURCEMANAGER.Get<Material>(L"Anim_Fanthor_Attack_010S");
			material2->GetParams().ExtValue[0] = Vec4(0.7f, 0.3f, 0.6f, 1.f); // RimPower
			material2->SetShader(L"Solid");
			material2s.push_back(material2);


			material2 = RESOURCEMANAGER.Get<Material>(L"Anim_Fanthor_Attack_011")->Clone();
			RESOURCEMANAGER.Add<Material>(L"Anim_Fanthor_Attack_011S", material2);
			material2 = RESOURCEMANAGER.Get<Material>(L"Anim_Fanthor_Attack_011S");
			material2->SetShader(L"Solid");
			material2->GetParams().ExtValue[0] = Vec4(0.6f, 0.2f, 0.5f, 1.f); // RimPower
			material2s.push_back(material2);
			anmators0.push_back(RESOURCEMANAGER.Get<Animator>(L"Anim_Fanthor_Idle"));
			anmators0.push_back(RESOURCEMANAGER.Get<Animator>(L"Anim_Fanthor_Walk"));
			//mWorld->AddComponent<MannequinComponent>(mEntityID, i);
			t.mLocalPosition = { 18.0f, 454.0,-4270.0f };
			t.mLocalRotationE = { 0.f, 90.f, 0.f };
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
#if USE_CPU_ANIMATION
	mWorld->GetSystemManager()->RegisterSystem<CpuAnimationSystem>();
#else
	mWorld->GetSystemManager()->RegisterSystem<AnimationSystem>();
#endif
	mWorld->GetSystemManager()->RegisterSystem<UIMainMenuSystem>();        // Post, After(UIButtonSystem), Before(MainMenuCameraSystem)
	mWorld->GetSystemManager()->RegisterSystem<MainMenuCameraSystem>();  // Post, Before(CameraSystem)
	mWorld->GetSystemManager()->RegisterSystem<CameraSystem>();
	mWorld->GetSystemManager()->RegisterSystem<AudioVisualizerSystem>();
	mWorld->GetSystemManager()->RegisterSystem<UITransformSystem>();
	auto* uiUpdateSystem = mWorld->GetSystemManager()->RegisterSystem<UIUpdateSystem>();
	uiUpdateSystem->SetFeatures(&mUIFeatures);

	mWorld->GetSystemManager()->RegisterSystem<UIButtonSystem>();
	mWorld->GetSystemManager()->RegisterSystem<AudioSystem>();
	//mWorld->GetSystemManager()->RegisterSystem<SocketTrailSystem>();
	mWorld->GetSystemManager()->RegisterSystem<ParticleSystem>();
	auto* renderSystemMM = mWorld->GetSystemManager()->RegisterSystem<RenderSystem>();
	renderSystemMM->SetPipeline(make_shared<GameRenderPipeline>());

	auto* uiRenderSystem = mWorld->GetSystemManager()->RegisterSystem<UIRenderSystem>();
	uiRenderSystem->SetFeatures(&mUIFeatures);

	mSceneId = SceneId::MainMenu;


}





void LobbyScene::Initialize()
{
	//PlayerPrefab player{mWorld.get()};
	mWorld->SetSceneId(mSceneId);
	PrefabFactory::RegisterAllPrefabs();
	//TerrainPrefab terrain{ mWorld.get() };
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
			material2 = RESOURCEMANAGER.Get<Material>(L"Anim_Rudwig_Base0");
			material2s.push_back(material2);
			material2 = RESOURCEMANAGER.Get<Material>(L"Anim_Rudwig_Base1");
			material2s.push_back(material2);
			/*material2 = RESOURCEMANAGER.Get<Material>(L"Anim_Rudwig_Idle0");
			material2s.push_back(material2);*/
			anmators0.push_back(RESOURCEMANAGER.Get<Animator>(L"Anim_Rudwig_Idle"));
			anmators0.push_back(RESOURCEMANAGER.Get<Animator>(L"Anim_Rudwig_Run"));
			mWorld->AddComponent<MannequinComponent>(mEntityID, i);
			break;
		case 1:
			phereMesh = RESOURCEMANAGER.Get<Mesh>(L"SM_Ibanix_Body");
			material2 = RESOURCEMANAGER.Get<Material>(L"Anim_Ibanix_Base0");
			material2s.push_back(material2);
			material2 = RESOURCEMANAGER.Get<Material>(L"Anim_Ibanix_Base1");
			material2s.push_back(material2);
			anmators0.push_back(RESOURCEMANAGER.Get<Animator>(L"Anim_Ibanix_Idle"));
			anmators0.push_back(RESOURCEMANAGER.Get<Animator>(L"Anim_Ibanix_Walk"));
			mWorld->AddComponent<MannequinComponent>(mEntityID, i);
			break;
		case 2:
			phereMesh = RESOURCEMANAGER.Get<Mesh>(L"SM_Fanthor_Body");
			material2 = RESOURCEMANAGER.Get<Material>(L"Anim_Fanthor_Base0");
			material2s.push_back(material2);
			material2 = RESOURCEMANAGER.Get<Material>(L"Anim_Fanthor_Base1");
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
		shared_ptr<Vfx> vfx = RESOURCEMANAGER.Get<Vfx>(L"VFX_Sector_Jump");
		mWorld->AddComponent<TransformComponent>(vfxEntity, vfxTransform);
		VfxComponent& vfxComp = mWorld->AddComponent<VfxComponent>(vfxEntity);
		vfxComp.mVfx = vfx;

	}
	/////////////////////////////////////////////////////////////////////





	/////////////////////////////////////////////////////////////////////////



	/////////////////////////////////////////////////////////////////////////



	mWorld->Initialize();

	// INPUT
	mWorld->GetSystemManager()->RegisterSystem<PlayerInputSystem>();

	// NETWORK
	mWorld->GetSystemManager()->RegisterSystem<NetRecvSystem>(mWorld->GetNetIdMap());
	mWorld->GetSystemManager()->RegisterSystem<NetSendSystem>();
#if USE_CPU_ANIMATION
	mWorld->GetSystemManager()->RegisterSystem<CpuAnimationSystem>();
#else
	mWorld->GetSystemManager()->RegisterSystem<AnimationSystem>();
#endif
	mWorld->GetSystemManager()->RegisterSystem<CameraSystem>();
	mWorld->GetSystemManager()->RegisterSystem<EnemySystem>();
	mWorld->GetSystemManager()->RegisterSystem<TransformSystem>();
	mWorld->GetSystemManager()->RegisterSystem<MovementSystem>();
	mWorld->GetSystemManager()->RegisterSystem<DamageFeedbackSystem>();
	mWorld->GetSystemManager()->RegisterSystem<AudioVisualizerSystem>();
	mWorld->GetSystemManager()->RegisterSystem<UITransformSystem>();
	auto* uiUpdateSystem = mWorld->GetSystemManager()->RegisterSystem<UIUpdateSystem>();
	uiUpdateSystem->SetFeatures(&mUIFeatures);

	mWorld->GetSystemManager()->RegisterSystem<AudioSystem>();
	mWorld->GetSystemManager()->RegisterSystem<BeatSystem>();
	mWorld->GetSystemManager()->RegisterSystem<NetInterpolationSystem>();
	mWorld->GetSystemManager()->RegisterSystem<VfxSystem>();
	auto* renderSystemLB = mWorld->GetSystemManager()->RegisterSystem<RenderSystem>();
	renderSystemLB->SetPipeline(make_shared<LobbyRenderPipeline>());
	auto* uiRenderSystem = mWorld->GetSystemManager()->RegisterSystem<UIRenderSystem>();
	uiRenderSystem->SetFeatures(&mUIFeatures);



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
		vis.basePosition = Vec2(2560.f / 2, 700.f);  // 화면 하단 중앙
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

	// 로비 Room 시스템
	{
		Entity roomStateEntity = mWorld->CreateEntity();
		mWorld->AddComponent<LobbyRoomStateComponent>(roomStateEntity);
	}


	mUIFeatures.push_back(std::make_shared<LobbyRoomUIFeature>());
	mUIFeatures.push_back(std::make_shared<DamagePopupUpdateFeature>());

	for (const auto& feature : mUIFeatures)
	{
		if (feature != nullptr)
			feature->Initialize(mWorld.get());
	}
}

#pragma endregion

/// //////////////////////////////////////////////////////////////////////////////////

/// //////////////////////////////////////////////////////////////////////////////////



#pragma region Game Scenes
void FirstScene::Initialize()
{
	mWorld->SetSceneId(mSceneId);
	//PlayerPrefab player{mWorld.get()};
	PrefabFactory::RegisterAllPrefabs();
	//TerrainPrefab terrain{ mWorld.get() };
	SkyBoxPrefab skybox{ mWorld.get() };
	DirLightPrefab light{ mWorld.get() };
	//EnemyPrefab	enemys {mWorld.get() };
	AreaConquestPrefab areaConquest{ mWorld.get() };

	// MAP export json load
	// [참고] 현재 FBX LOADER에서 NormalMap을 읽지 못하게 함.
	// 
		//LoadJsonLevel(L"..\\Resources\\Json\\M_StylizedStudyLogCabin_A1_Export.json");
		// LoadJsonLevel(L"..\\Resources\\Json\\ThirdPersonMap_Export.json");
	LoadJsonLevelData(L"..\\Resources\\Json\\Map001_Export.json");
	LoadCollisionJson(L"..\\Resources\\Json\\Map001_Nav_Export.json");

	/////////////////////////////////////////////////////////////////////
	//{
	//	Entity vfxEntity = mWorld->CreateEntity();
	//	TransformComponent vfxTransform{};
	//	vfxTransform.mLocalPosition = Vec3(0.f, 35.f, 0.f);
	//	shared_ptr<Vfx> vfx = RESOURCEMANAGER.Get<Vfx>(L"vfx_dissolve_NoteBoar");
	//	mWorld->AddComponent<TransformComponent>(vfxEntity, vfxTransform);
	//	VfxComponent& vfxComp = mWorld->AddComponent<VfxComponent>(vfxEntity);
	//	vfxComp.mVfx = vfx;
	//}
	//{
	//	
	//	Entity particleEntity = mWorld->CreateEntity();
	//	TransformComponent particleTransform{};
	//	particleTransform.mLocalPosition = Vec3(0.f, 120.f, 0.f);
	//	mWorld->AddComponent<TransformComponent>(particleEntity, particleTransform);

	//	ParticleComponent& particle = mWorld->AddComponent<ParticleComponent>(particleEntity);
	//	particle.mEffectName = L"Particle_DebugBurst";
	//}
	//


	/////////////////////////////////////////////////////////////////////
	// [ 샘플 ]

	{
		/*Entity enityt = mWorld->CreateEntity();
		TransformComponent t{};
		t.mLocalPosition = Vec3(1500.f, 720.f, 0.f);
		mWorld->AddComponent<TransformComponent>(enityt, t);
		shared_ptr<Mesh> mesh = RESOURCEMANAGER.Get<Mesh>(L"SM_Rudwig_Body");
		shared_ptr<Material> material = RESOURCEMANAGER.Get<Material>(L"Anim_Rudwig_Idle0");
		std::vector<shared_ptr<Material>> materials;
		materials.push_back(material);
		mWorld->AddComponent<RenderComponent>(enityt, mesh, materials);*/

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
#pragma region Game Objects
	// 점프대
	{
		Entity jumpEntity = mWorld->CreateEntity();
		TransformComponent vfxTransform{};
		vfxTransform.mLocalPosition = Vec3(-2337.f, 142.f, -4987.f);

		shared_ptr<Vfx> vfx = RESOURCEMANAGER.Get<Vfx>(L"VFX_Sector_Jump");

		mWorld->AddComponent<TransformComponent>(jumpEntity, vfxTransform);
		VfxComponent& vfxComp = mWorld->AddComponent<VfxComponent>(jumpEntity);
		vfxComp.mVfx = vfx;
		vfxComp.mIsLoop = true;
		vfxComp.mRestartWhenFinished = true;
		vfxComp.mScale = Vec3(15.f, 10.f, 15.f);
	}

	// 힐팩
	{
		Entity healEntity = mWorld->CreateEntity();
		TransformComponent vfxTransform{};
		vfxTransform.mLocalPosition = Vec3(-5843.f, 278.f, -3523.f);

		shared_ptr<Vfx> vfx = RESOURCEMANAGER.Get<Vfx>(L"VFX_Sector_Heal");

		mWorld->AddComponent<TransformComponent>(healEntity, vfxTransform);
		VfxComponent& vfxComp = mWorld->AddComponent<VfxComponent>(healEntity);
		vfxComp.mVfx = vfx;
		vfxComp.mIsLoop = true;
		vfxComp.mRestartWhenFinished = true;
		vfxComp.mScale = Vec3(5.f, 5.f, 5.f);
	}

	// 몬스터 스포너
	{
		Entity spawnMonEntity1 = mWorld->CreateEntity();
		TransformComponent vfxTransform{};
		vfxTransform.mLocalPosition = Vec3(-4910.0f, 142.0f, -1623.0f);

		shared_ptr<Vfx> vfx = RESOURCEMANAGER.Get<Vfx>(L"VFX_Sector_Spawn");

		mWorld->AddComponent<TransformComponent>(spawnMonEntity1, vfxTransform);
		VfxComponent& vfxComp = mWorld->AddComponent<VfxComponent>(spawnMonEntity1);
		vfxComp.mVfx = vfx;
		vfxComp.mIsLoop = true;
		vfxComp.mRestartWhenFinished = true;
		vfxComp.mScale = Vec3(5.f, 5.f, 5.f);
	}
	{
		Entity spawnMonEntity2 = mWorld->CreateEntity();
		TransformComponent vfxTransform{};
		vfxTransform.mLocalPosition = Vec3(-2307.0f, 740.0f, -4097.0f);

		shared_ptr<Vfx> vfx = RESOURCEMANAGER.Get<Vfx>(L"VFX_Sector_Spawn");

		mWorld->AddComponent<TransformComponent>(spawnMonEntity2, vfxTransform);
		VfxComponent& vfxComp = mWorld->AddComponent<VfxComponent>(spawnMonEntity2);
		vfxComp.mVfx = vfx;
		vfxComp.mIsLoop = true;
		vfxComp.mRestartWhenFinished = true;
		vfxComp.mScale = Vec3(5.f, 5.f, 5.f);
	}
	{
		Entity spawnMonEntity3 = mWorld->CreateEntity();
		TransformComponent vfxTransform{};
		vfxTransform.mLocalPosition = Vec3(-5943.f, 142.0f, -5637.0f);

		shared_ptr<Vfx> vfx = RESOURCEMANAGER.Get<Vfx>(L"VFX_Sector_Spawn");

		mWorld->AddComponent<TransformComponent>(spawnMonEntity3, vfxTransform);
		VfxComponent& vfxComp = mWorld->AddComponent<VfxComponent>(spawnMonEntity3);
		vfxComp.mVfx = vfx;
		vfxComp.mIsLoop = true;
		vfxComp.mRestartWhenFinished = true;
		vfxComp.mScale = Vec3(5.f, 5.f, 5.f);
	}

#pragma endregion

#pragma region UI

	//{

	//	shared_ptr<Texture> texture = RESOURCEMANAGER.Get<Texture>(L"fire");
	//	Entity fire = mWorld->CreateEntity();
	//	auto& t = mWorld->AddComponent<UISpriteComponent>(fire, texture,
	//		Vec2(64.f, 64.f), 4, 1.f);
	//	auto& u = mWorld->AddComponent<UITransformComponent>(fire);
	//	u.mAnchor = Anchor::Center;
	//	u.mPosition = Vec2(0.f, 0.f);
	//	u.mSize = Vec2(64, 64);

	//}



	/*{
		Entity Ibanix_Ammo = mWorld->CreateEntity();
		shared_ptr<Material> scorem;
		scorem = RESOURCEMANAGER.Get<Material>(L"Ibanix_Ammo");

		auto& t = mWorld->AddComponent<UITransformComponent>(Ibanix_Ammo);
		t.mAnchor = Anchor::BottomLeft;
		t.mPosition = Vec2(212.f, -212.f);
		t.mSize = Vec2(196.f, 128.f);

		auto& m = mWorld->AddComponent<UICusSpriteComponent>(Ibanix_Ammo, scorem);
	}*/

	//	{
	//		Entity Fanthor_Portrait = mWorld->CreateEntity();
	//
	//		shared_ptr<Material> scorem;
	//		scorem = RESOURCEMANAGER.Get<Material>(L"Fanthor_Portrait");
	//
	//		auto& t = mWorld->AddComponent<UITransformComponent>(Fanthor_Portrait);
	//		t.mAnchor = Anchor::BottomLeft;
	//		t.mPosition = Vec2(32.f, -636.f);
	//		t.mSize = Vec2(196.f, 196.f);
	//		t.mPivot = Vec2(0.5f, 0.5f);
	//
	//		auto& m = mWorld->AddComponent<UIActionComponent>(Fanthor_Portrait);
	//		m.mDuration = 0.5f;
	//		m.mActor = UIActor::Player;
	//		m.mState = UIActionState::Bounce;
	//		m.mIsLoop = true;
	//		m.mBounceAmplitude =20.f;
	//		m.mBounceFrequency = 2.f;
	//		m.mBounceDamping = 10.f;
	//		mWorld->AddComponent<UICusSpriteComponent>(Fanthor_Portrait, scorem);
	//
	//#ifdef _IMGUI
	//
	//		std::vector<EditorProperty> props;
	//		props.push_back({ "Fanthor_Portrait Position1",  PropertyType::Vec2,  &(t.mPosition),  0.f,    0.f });
	//		props.push_back({ "Fanthor_Portrait Bounce",  PropertyType::Float,  &(m.mBounceDamping),  -10.f, 10.f});
	//		IMGUIComponent& visImgui = mWorld->AddComponent<IMGUIComponent>(Fanthor_Portrait);
	//		visImgui.RegisterEditorProperties(props);
	//		visImgui.SetName("Menu");
	//#endif
	//
	//	}

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





	auto audioVisualizerModule = std::make_shared<UIAudioVisualizerFeature>();
	mUIFeatures.push_back(audioVisualizerModule);

	auto actionModule = std::make_shared<UIActionUpdateFeature>();
	mUIFeatures.push_back(actionModule);

	auto hpBarModule = std::make_shared<UIHpBarUpdateFeature>();
	mUIFeatures.push_back(hpBarModule);

	auto damagePopupModule = std::make_shared<DamagePopupUpdateFeature>();
	mUIFeatures.push_back(damagePopupModule);

	auto gameInfoModule = std::make_shared<UIGameInfoUpdateFeature>();
	mUIFeatures.push_back(gameInfoModule);

	auto gameProgressModule = std::make_shared<UIPhaseProgressUpdateFeature>();
	mUIFeatures.push_back(gameProgressModule);


	for (const auto& feature : mUIFeatures)
	{
		if (feature != nullptr)
			feature->Initialize(mWorld.get());
	}


#pragma endregion

	/////////////////////////////////////////////////////////////////////////


#pragma region Systems
	mWorld->Initialize();

	// INPUT
	mWorld->GetSystemManager()->RegisterSystem<PlayerInputSystem>();

	// NETWORK
	mWorld->GetSystemManager()->RegisterSystem<NetRecvSystem>(mWorld->GetNetIdMap());
	mWorld->GetSystemManager()->RegisterSystem<NetSendSystem>();
	mWorld->GetSystemManager()->RegisterSystem<GamePhaseSystem>();
#if USE_CPU_ANIMATION
	mWorld->GetSystemManager()->RegisterSystem<CpuAnimationSystem>();
#else
	mWorld->GetSystemManager()->RegisterSystem<AnimationSystem>();
#endif
	mWorld->GetSystemManager()->RegisterSystem<CameraSystem>();

	mWorld->GetSystemManager()->RegisterSystem<EnemySystem>();
	mWorld->GetSystemManager()->RegisterSystem<PlayerSystem>();
	mWorld->GetSystemManager()->RegisterSystem<TransformSystem>();
	mWorld->GetSystemManager()->RegisterSystem<MovementSystem>();
	mWorld->GetSystemManager()->RegisterSystem<DamageFeedbackSystem>();
	mWorld->GetSystemManager()->RegisterSystem<AudioVisualizerSystem>();
	mWorld->GetSystemManager()->RegisterSystem<UITransformSystem>();
	auto* uiUpdateSystem = mWorld->GetSystemManager()->RegisterSystem<UIUpdateSystem>();
	uiUpdateSystem->SetFeatures(&mUIFeatures);
	mWorld->GetSystemManager()->RegisterSystem<AudioSystem>();
	mWorld->GetSystemManager()->RegisterSystem<BeatSystem>();
	mWorld->GetSystemManager()->RegisterSystem<NetInterpolationSystem>();
	//mWorld->GetSystemManager()->RegisterSystem<SocketTrailSystem>();
	mWorld->GetSystemManager()->RegisterSystem<VfxSystem>();
	mWorld->GetSystemManager()->RegisterSystem<ParticleSystem>();


	auto* renderSystemFS = mWorld->GetSystemManager()->RegisterSystem<RenderSystem>();
	renderSystemFS->SetPipeline(make_shared<GameRenderPipeline>());
	shared_ptr<GameRenderPipeline> gp = static_pointer_cast<GameRenderPipeline>(renderSystemFS->GetPipeline());
	gp->SetWorldUIFeature(&mUIFeatures);


	auto* uiRenderSystem = mWorld->GetSystemManager()->RegisterSystem<UIRenderSystem>();
	uiRenderSystem->SetFeatures(&mUIFeatures);

#pragma endregion



	mWorld->AddComponent<GameRuleComponent>(mWorld->GetGameRuleEntity());



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
	LoadCollisionJson(L"..\\Resources\\Json\\Map001_CRX.json");

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
