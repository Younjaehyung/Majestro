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
#include "CircularVisualizerComponent.h"
#include "EffectFlagComponent.h"
#include "Prefab.h"
#include "LobbyRoomStateComponent.h"
#include "LobbyRoomListComponent.h"
#include "LobbyRoomSystem.h"
#include "LobbyRoomBrowserFeature.h"
#include "MenuRoomBrowserSystem.h"

#include "RenderSystem.h"
#include "GameRenderPipeline.h"
#include "LobbyRenderPipeline.h"
#include "CameraSystem.h"
#include "AudioSystem.h"
#include "TransformSystem.h"
#include "AnimationSystem.h"
#include "CpuAnimationSystem.h"
#include "SocketSystem.h"
#include "SocketFollowSystem.h"
#include "WeaponTrailSystem.h"
#include "AnimNotifySystem.h"
#include "DashSpeedLineSystem.h"
#include "PlayerSystem.h"
#include "SpectateSystem.h"
#include "ParticleSystem.h"
#include "VfxSystem.h"
#include "SfxSystem.h"

#include "UIRenderSystem.h"
#include "UIUpdateSystem.h"
#include "HUDPortraitUpdateFeature.h"
#include "HUDSkillCooldownFeature.h"
#include "IMGUISystem.h"
#include "BeatSystem.h"
#include "MovementSystem.h"
#include "GamePhaseSystem.h"
#include "DamageFeedbackSystem.h"
#include "RhythmSystem.h"
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

#include "PauseMenuController.h"
#include "PauseSystem.h"



namespace
{
	// 맵 FBX 경로
	std::wstring BuildMapFbxPath(const std::string& levelName, const std::string& stem)
	{
		if (!levelName.empty())
		{
			std::string subBase = "..\\Resources\\Map\\" + levelName + "\\" + stem;
			if (std::filesystem::exists(subBase + ".mesh"))
				return s2ws(subBase + ".fbx");   // 경로는 .fbx 로 넘기지만 로더가 parent\stem.mesh 를 읽음
		}
		return s2ws("..\\Resources\\Map\\" + stem + ".fbx");
	}
}

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

	const std::wstring prefix = s2ws(level.levelName);
	for (const auto& fbxName : uniqueFbxNames)
	{
		RESOURCEMANAGER.LoadFBXModel(BuildMapFbxPath(level.levelName, fbxName), prefix);
	}

	RESOURCEMANAGER.ApplyMapSky(level);
}

void Scene::LoadJsonLevelData(const wstring& path) {
	int i = 0;
	try
	{
		LevelImportData level = RESOURCEMANAGER.LoadMapResourceJson(path);
		const std::wstring prefix = s2ws(level.levelName);

		RESOURCEMANAGER.ApplyMapSky(level);

		for (const auto& inst : level.instances)
		{
			// 파일명만 추출
			std::string name = filesystem::path(inst.fbx).filename().stem().string();

			shared_ptr<FBXData> data = RESOURCEMANAGER.Get<FBXData>(ResourceManager::MakeKey(prefix, s2ws(name)));
		
			if (!data)
			{
				std::cerr << "FBX load failed (null data): " << name << "\n";
				break;
			}
			else if (data->GetMeshs().empty()) {
				std::cerr << "FBX load failed Mesh (null data): " << name << "\n";
				continue;
			}

			const auto& meshes = data->GetMeshs();
			const auto& meshMats = data->GetMeshMaterials();
			for (size_t mi = 0; mi < meshes.size(); ++mi)
			{
				if (!meshes[mi]) continue;

				Entity entity = mWorld->CreateEntity();
				TransformComponent transform{};
				transform.mWorldMatrix = inst.worldMtx;
				TransformComponent& trans = mWorld->AddComponent<TransformComponent>(entity, transform);
				trans.mIsStatic = true;

				RenderComponent& render = mWorld->AddComponent<RenderComponent>(entity);
				if (mi < meshMats.size())
					render.mMaterials = meshMats[mi];
				render.SetMesh(meshes[mi]);
			}
			i++;
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
		const std::wstring prefix = s2ws(level.levelName);

		RESOURCEMANAGER.ApplyMapSky(level);

		for (const auto& inst : level.instances)
		{
			// 파일명만 추출
			std::string stem = filesystem::path(inst.fbx).filename().stem().string();
			shared_ptr<FBXData> data = RESOURCEMANAGER.LoadFBXModel(BuildMapFbxPath(level.levelName, stem), prefix);
			
			if (!data)
			{
				std::cerr << "FBX load failed (null data): " << stem << "\n";
				break;
			}
			else if (data->GetMeshs().empty()) {
				std::cerr << "FBX load failed Mesh (null data): " << stem << "\n";
				continue;
			}


			const auto& meshes = data->GetMeshs();
			const auto& meshMats = data->GetMeshMaterials();
			for (size_t mi = 0; mi < meshes.size(); ++mi)
			{
				if (!meshes[mi]) continue;

				Entity entity = mWorld->CreateEntity();
				TransformComponent transform{};
				transform.mWorldMatrix = inst.worldMtx;
				TransformComponent& trans = mWorld->AddComponent<TransformComponent>(entity, transform);
				trans.mIsStatic = true;

				RenderComponent& render = mWorld->AddComponent<RenderComponent>(entity);
				if (mi < meshMats.size())
					render.mMaterials = meshMats[mi];
				render.SetMesh(meshes[mi]);
			}
			i++;
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
		const std::wstring prefix = s2ws(level.levelName);
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

			std::wstring fbxPath = BuildMapFbxPath(level.levelName, stem);
			shared_ptr<FBXData> collisionFbx = RESOURCEMANAGER.LoadFBXMesh(fbxPath, prefix);
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

void Scene::CreatePauseMenu()
{
	/////////////////////////////////////////////////////////////////////////
	// 일시정지(Pause) 메뉴 — ESC 키로 토글.

#pragma region Pause Menu
	{
		const Vec2 btnSize = { 320.f, 72.f };

		// FSM 컨트롤러
		Entity pauseCtrlEnt = mWorld->CreateEntity();
		mWorld->AddComponent<PauseMenuController>(pauseCtrlEnt);

		auto requestPause = [this, pauseCtrlEnt](PauseMenuState s)
			{
				if (auto* c = mWorld->GetComponent<PauseMenuController>(pauseCtrlEnt))
					c->Request(s);
			};

		// 풀스크린 배경
		Entity pauseDim = mWorld->CreateEntity();
		{
			auto& tr = mWorld->AddComponent<UITransformComponent>(pauseDim);
			tr.mAnchor = Anchor::TopLeft;
			tr.mPosition = Vec2(0.f, 0.f);
			tr.mSize = Vec2(2560.f, 1440.f);
			tr.mPivot = Vec2(0.f, 0.f);
			tr.mUILayerIndex = 1;            // 버튼(5)보다 뒤
			auto& sp = mWorld->AddComponent<UISpriteComponent>(pauseDim,
				RESOURCEMANAGER.Get<Texture>(L"UI_Loading_Main_01"));
			sp.mColorTint = Vec4(0.f, 0.f, 0.f, 0.6f);
		}

		// Pause: 타이틀 + Resume / Setting / Disconnect
		Entity pauseTitle = mWorld->CreateEntity();
		{
			auto& tr = mWorld->AddComponent<UITransformComponent>(pauseTitle);
			tr.mAnchor = Anchor::Center;
			tr.mPosition = Vec2(0.f, -260.f);
			tr.mSize = Vec2(400.f, 80.f);
			tr.mPivot = Vec2(0.5f, 0.5f);
			tr.mUILayerIndex = 5;
			auto& txt = mWorld->AddComponent<UITextComponent>(pauseTitle);
			txt.mText = L"PAUSED";
		}
		Entity bResume = CreateUIButton(mWorld.get(), {
			.position = Vec2(0.f, -100.f),
			.size = btnSize,
			.visual = UIButtonVisual::Vfx,
			.resKey = L"VFX_UI_Select",
			.label = L"RESUME",
			.onClick = [this, pauseCtrlEnt]()
			{
				if (auto* c = mWorld->GetComponent<PauseMenuController>(pauseCtrlEnt))
				{
					c->mPaused = false;
					c->Request(PauseMenuState::Hidden);
				}
			},
			});
		Entity bSetting = CreateUIButton(mWorld.get(), {
			.position = Vec2(0.f, 0.f),
			.size = btnSize,
			.visual = UIButtonVisual::Vfx,
			.resKey = L"VFX_UI_Select",
			.label = L"SETTING",
			.onClick = [requestPause]() { requestPause(PauseMenuState::Setting); },
			});
		Entity bDisconnect = CreateUIButton(mWorld.get(), {
			.position = Vec2(0.f, 100.f),
			.size = btnSize,
			.visual = UIButtonVisual::Vfx,
			.resKey = L"VFX_UI_Select",
			.label = L"DISCONNECT",
			.onClick = [requestPause]() { requestPause(PauseMenuState::ConfirmDisconnect); },
			});

		// Setting: Back (실제 옵션 UI 는 추후)
		Entity settingText = mWorld->CreateEntity();
		{
			auto& tr = mWorld->AddComponent<UITransformComponent>(settingText);
			tr.mAnchor = Anchor::Center;
			tr.mPosition = Vec2(0.f, -100.f);
			tr.mSize = Vec2(600.f, 80.f);
			tr.mPivot = Vec2(0.5f, 0.5f);
			tr.mUILayerIndex = 5;
			auto& txt = mWorld->AddComponent<UITextComponent>(settingText);
			txt.mText = L"SETTING (준비 중)";
		}
		Entity bSettingBack = CreateUIButton(mWorld.get(), {
			.position = Vec2(0.f, 80.f),
			.size = btnSize,
			.visual = UIButtonVisual::Vfx,
			.resKey = L"VFX_UI_Select",
			.label = L"BACK",
			.onClick = [requestPause]() { requestPause(PauseMenuState::Root); },
			.clickSfxKey = "ui/back",
			});

		// ConfirmDisconnect: Yes / No
		Entity confirmText = mWorld->CreateEntity();
		{
			auto& tr = mWorld->AddComponent<UITransformComponent>(confirmText);
			tr.mAnchor = Anchor::Center;
			tr.mPosition = Vec2(0.f, -100.f);
			tr.mSize = Vec2(800.f, 80.f);
			tr.mPivot = Vec2(0.5f, 0.5f);
			tr.mUILayerIndex = 5;
			auto& txt = mWorld->AddComponent<UITextComponent>(confirmText);
			txt.mText = L"게임에서 나가시겠습니까?";
		}
		Entity bYes = CreateUIButton(mWorld.get(), {
			.position = Vec2(-180.f, 80.f),
			.size = btnSize,
			.visual = UIButtonVisual::Vfx,
			.resKey = L"VFX_UI_Select",
			.label = L"YES",
			.onClick = [this]()
			{
				Network::GetInstance().Shutdown();
				mGameMode->mTargetSceneId = SceneId::MainMenu;
				mGameMode->IsSceneChanging() = true;
			},
			});
		Entity bNo = CreateUIButton(mWorld.get(), {
			.position = Vec2(180.f, 80.f),
			.size = btnSize,
			.visual = UIButtonVisual::Vfx,
			.resKey = L"VFX_UI_Select",
			.label = L"NO",
			.onClick = [requestPause]() { requestPause(PauseMenuState::Root); },
			.clickSfxKey = "ui/back",
			});

		// 상태별 entity 등록 (Hidden 은 비움). 배경은 세 상태 공유.
		auto* pctrl = mWorld->GetComponent<PauseMenuController>(pauseCtrlEnt);
		pctrl->mStateEntities[(size_t)PauseMenuState::Root] = { pauseDim, pauseTitle, bResume, bSetting, bDisconnect };
		pctrl->mStateEntities[(size_t)PauseMenuState::Setting] = { pauseDim, settingText, bSettingBack };
		pctrl->mStateEntities[(size_t)PauseMenuState::ConfirmDisconnect] = { pauseDim, confirmText, bYes, bNo };

		// 시작 시 모든 일시정지 UI 숨김
		auto hidePause = [this](Entity e)
			{
				if (auto* sp = mWorld->GetComponent<UISpriteComponent>(e)) sp->mVisible = false;
				if (auto* vf = mWorld->GetComponent<UIVfxComponent>(e))    vf->mVisible = false;
				if (auto* tx = mWorld->GetComponent<UITextComponent>(e))   tx->mVisible = false;
				if (auto* bt = mWorld->GetComponent<UIButtonComponent>(e)) bt->mEnabled = false;
			};
		for (int s = 0; s < (int)PauseMenuState::Count; ++s)
			for (Entity e : pctrl->mStateEntities[s])
				hidePause(e);
	}
#pragma endregion


	/////////////////////////////////////////////////////////////////////////

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
	AUDIOMANAGER.RequestBGM("event:/OST/Loading", SOUNDNAME::Ambient);
	AUDIOMANAGER.Update(0.f);

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
		tr.mSize = Vec2(2560.f, 1440.f);
		tr.mPivot = Vec2(0.f, 0.f);

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
	mWorld->GetSystemManager()->RegisterSystem<SfxSystem>();
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
		mapPath = L"..\\Resources\\Json\\MapDesert_Export.json";
	}
								   break;
	case (uint8)SceneId::ThirdGame: {
		mTotalTaskCount = 0;	// 임시
		return true;
	}
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

	const std::string levelName = level.levelName;
	for (const auto& fbxName : uniqueFbxNames)
	{
		mLoadTasks.push([fbxName, levelName]() {
			shared_ptr<FBXData> data = RESOURCEMANAGER.LoadFBXModel(BuildMapFbxPath(levelName, fbxName), s2ws(levelName));});
	}

	mTotalTaskCount = (int32)mLoadTasks.size();


	AUDIOMANAGER.RequestBGM("event:/OST/Loading", SOUNDNAME::Ambient);
	AUDIOMANAGER.Update(0.f);

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

	mWorld->Update(deltaTime);
}

#pragma endregion

//////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////

#pragma region Menu Scenes

void MainMenuScene::Initialize()
{
	mWorld->SetSceneId(mSceneId);

	
	AUDIOMANAGER.RequestBGM("event:/OST/Menu", SOUNDNAME::Ambient);
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
	// 메인메뉴 
	// 아래 배경(1) 애니메이션 배경(2) 버튼(5) 위
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
				requestState(MainMenuState::RoomList);	// RoomList 상태로 전환
				Network::GetInstance().Awake();			// 서버 접속
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
			.clickSfxKey = "ui/back",
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
			.clickSfxKey = "ui/back",
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
		ctrl->mStates[(size_t)MainMenuState::Title].entities = { titleHint };
		ctrl->mStates[(size_t)MainMenuState::MainMenu].entities = { bGameStart, bManual, bSetting, bMainExit };
		ctrl->mStates[(size_t)MainMenuState::Setting].entities = { bBack, bSubExit };
		ctrl->mStates[(size_t)MainMenuState::Manual].entities = { bBack, bSubExit };
		ctrl->mStates[(size_t)MainMenuState::RoomList].entities = { bBack, bSubExit };
		ctrl->mStates[(size_t)MainMenuState::Exit].entities = { bYes, bNo };

		// 상태별 풀스크린 배경
		auto makeBg = [this](const std::wstring& texKey) -> Entity
			{
				Entity e = mWorld->CreateEntity();
				auto& tr = mWorld->AddComponent<UITransformComponent>(e);
				tr.mLayoutMode = UILayoutMode::ScreenRatio;     // 어떤 종횡비든 화면 꽉 채움
				tr.mAnchor = Anchor::Center;
				tr.mPositionRatio = Vec2(0.f, 0.f);
				tr.mSizeRatio = Vec2(1.f, 1.f);
				tr.mPivot = Vec2(0.5f, 0.5f);                   // 중앙 기준 줌
				tr.mUILayerIndex = 1;                           // 버튼/텍스트보다 뒤
				auto& sp = mWorld->AddComponent<UISpriteComponent>(e, RESOURCEMANAGER.Get<Texture>(texKey));
				sp.mVisible = false;                            // 페이드 시작 전 숨김
				sp.mColorTint = Vec4(1.f, 1.f, 1.f, 0.f);       // 알파 0 시작
				return e;
			};
		ctrl->mStates[(size_t)MainMenuState::MainMenu].background = makeBg(L"UI_Title_SelectFrame");
		ctrl->mStates[(size_t)MainMenuState::Setting].background  = makeBg(L"UI_Title_Setting");
		ctrl->mStates[(size_t)MainMenuState::Manual].background   = makeBg(L"UI_Title_Control");
		ctrl->mStates[(size_t)MainMenuState::RoomList].background = makeBg(L"UI_Title_Search");
		ctrl->mStates[(size_t)MainMenuState::Exit].background     = makeBg(L"UI_Title_QuitGame");

		// 방에서 나와 복귀한 경우(서버 연결 유지 중) 타이틀을 건너뛰고 방 목록으로 직행
		if (Network::GetInstance().IsRunning())
			ctrl->Request(MainMenuState::RoomList);

		// 배경 장식 애니메이션
		auto makeAnim = [this](const std::wstring& texKey, const Vec2& pos, const Vec2& size,
		                       const Vec2& frameSize, int frameCount, float animTime) -> Entity
			{
				Entity e = mWorld->CreateEntity();
				auto& tr = mWorld->AddComponent<UITransformComponent>(e);
				tr.mAnchor       = Anchor::Center;
				tr.mPosition     = pos;
				tr.mSize         = size;
				tr.mPivot        = Vec2(0.5f, 0.5f);
				tr.mUILayerIndex = 2;                   
				auto& sp = mWorld->AddComponent<UISpriteComponent>(
					e, RESOURCEMANAGER.Get<Texture>(texKey), frameSize, frameCount, animTime);
				sp.mVisible = false;
				return e;
			};
	



		ctrl->mStates[(size_t)MainMenuState::MainMenu].animations = {
			makeAnim(L"UI_Title_PaintSplash_0", Vec2(0.f, 0.f), Vec2(2320.f, 464.f), Vec2(464.f, 464.f), 5, 2.0f),
		};
	
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
			for (Entity e : ctrl->mStates[s].entities)
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

	mWorld->GetSystemManager()->RegisterSystem<NetRecvSystem>(mWorld->GetNetIdMap());
	mWorld->GetSystemManager()->RegisterSystem<NetSendSystem>();
	mWorld->GetSystemManager()->RegisterSystem<MenuRoomBrowserSystem>();


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
	mWorld->GetSystemManager()->RegisterSystem<SfxSystem>();
	//mWorld->GetSystemManager()->RegisterSystem<SocketTrailSystem>();
	mWorld->GetSystemManager()->RegisterSystem<ParticleSystem>();
	auto* renderSystemMM = mWorld->GetSystemManager()->RegisterSystem<RenderSystem>();
	renderSystemMM->SetPipeline(make_shared<GameRenderPipeline>());

	auto* uiRenderSystem = mWorld->GetSystemManager()->RegisterSystem<UIRenderSystem>();
	uiRenderSystem->SetFeatures(&mUIFeatures);


	{
		Entity roomListEntity = mWorld->CreateEntity();
		mWorld->AddComponent<LobbyRoomListComponent>(roomListEntity);
	}

	mSceneId = SceneId::MainMenu;


}





void LobbyScene::Initialize()
{
	mWorld->SetSceneId(mSceneId);
	PrefabFactory::RegisterAllPrefabs();
	DirLightPrefab light{ mWorld.get() };

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
		// 미선택(0xFF)
		mWorld->AddComponent<ChoicePlayerComponent>(mannequinEntity, 0xFF);

	}

	for (int i = 0; i < PlayerType::Count; ++i) {
		Entity mEntityID = mWorld->CreateEntity();

		TransformComponent t{};
		shared_ptr<Mesh> phereMesh;
		shared_ptr<Material> material2;
		std::vector<shared_ptr<Material>> material2s;   // 일반 머티리얼(선택된 캐릭터)
		std::vector<Vec4> rimPowers;                     // Solid 셰이더용 RimPower (머티리얼별)
		vector<shared_ptr<Animator>> anmators0;

		switch (i) {
			case PlayerType::Rudwig:
				phereMesh = RESOURCEMANAGER.Get<Mesh>(L"SM_Rudwig_Body");
				material2 = RESOURCEMANAGER.Get<Material>(L"Anim_Rudwig_Base0");
				material2s.push_back(material2); rimPowers.push_back(Vec4(0.9f, 0.8f, 0.1f, 1.f));
				material2 = RESOURCEMANAGER.Get<Material>(L"Anim_Rudwig_Base1");
				material2s.push_back(material2); rimPowers.push_back(Vec4(0.8f, 0.7f, 0.0f, 1.f));
				anmators0.push_back(RESOURCEMANAGER.Get<Animator>(L"Anim_Rudwig_Idle"));
				anmators0.push_back(RESOURCEMANAGER.Get<Animator>(L"Anim_Rudwig_Run"));

				t.mLocalPosition = { -50.f, 0.f, 25.f };
				t.mLocalRotationE = {0.f,30.f,0.f};
				break;
			case PlayerType::Ibanix:
				phereMesh = RESOURCEMANAGER.Get<Mesh>(L"SM_Ibanix_Body");
				material2 = RESOURCEMANAGER.Get<Material>(L"Anim_Ibanix_Base0");
				material2s.push_back(material2); rimPowers.push_back(Vec4(0.2f, 0.6f, 0.2f, 1.f));
				material2 = RESOURCEMANAGER.Get<Material>(L"Anim_Ibanix_Base1");
				material2s.push_back(material2); rimPowers.push_back(Vec4(0.1f, 0.5f, 0.1f, 1.f));
				anmators0.push_back(RESOURCEMANAGER.Get<Animator>(L"Anim_Ibanix_Idle"));
				anmators0.push_back(RESOURCEMANAGER.Get<Animator>(L"Anim_Ibanix_Walk"));

				t.mLocalPosition = { 100.f, 0.f, 50.f };
				t.mLocalRotationE = { 0.f,30.f,0.f };
				break;
			case PlayerType::Fanthor:
				phereMesh = RESOURCEMANAGER.Get<Mesh>(L"SM_Fanthor_Body");
				material2 = RESOURCEMANAGER.Get<Material>(L"Anim_Fanthor_Base0");
				material2s.push_back(material2); rimPowers.push_back(Vec4(0.7f, 0.3f, 0.6f, 1.f));
				material2 = RESOURCEMANAGER.Get<Material>(L"Anim_Fanthor_Base1");
				material2s.push_back(material2); rimPowers.push_back(Vec4(0.6f, 0.2f, 0.5f, 1.f));
				anmators0.push_back(RESOURCEMANAGER.Get<Animator>(L"Anim_Fanthor_Idle"));
				anmators0.push_back(RESOURCEMANAGER.Get<Animator>(L"Anim_Fanthor_Walk"));

				t.mLocalPosition = { 250.f, 0.f, -25.f };
				t.mLocalRotationE = { 0.f,30.f,0.f };
				break;
		}

		// 타이틀 맵과 동일한 룩: 일반 머티리얼을 복제 후 Solid 셰이더 + RimPower 적용
		// (ResourceManager 에 키 등록하지 않아 타이틀 씬의 *S 키와 충돌하지 않음)
		std::vector<shared_ptr<Material>> solidMats;
		for (size_t m = 0; m < material2s.size(); ++m) {
			shared_ptr<Material> solid = material2s[m]->Clone();
			solid->SetShader(L"Solid");
			solid->GetParams().ExtValue[0] = rimPowers[m];
			solidMats.push_back(solid);
		}

		// 마네킹에 두 머티리얼 세트 보관 (선택 여부에 따라 CpuAnimationSystem 이 매 프레임 교체)
		auto& mann = mWorld->AddComponent<MannequinComponent>(mEntityID, i);
		mann.mNormalMaterials = material2s;
		mann.mSolidMaterials  = solidMats;

		// 초기 선택 캐릭터만 일반 머티리얼, 나머지는 Solid 로 시작
		uint8 choiceType = 1;
		if (auto cpEnts = mWorld->GetEntitiesWithComponent<ChoicePlayerComponent>(); !cpEnts.empty())
			if (auto* cp = mWorld->GetComponent<ChoicePlayerComponent>(cpEnts[0]))
				choiceType = cp->mPlayerType;
		std::vector<shared_ptr<Material>>& initMats =
			(static_cast<uint8>(i) == choiceType) ? material2s : solidMats;

		
		mWorld->AddComponent<TransformComponent>(mEntityID, t);
		mWorld->AddComponent<RenderComponent>(mEntityID, phereMesh, initMats);
		mWorld->AddComponent<AnimationComponent>(mEntityID, anmators0);
	}


	// MAP export json load
	// [참고] 현재 FBX LOADER에서 NormalMap을 읽지 못하게 함.
	// 

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
	mWorld->GetSystemManager()->RegisterSystem<LobbyRoomSystem>();
	mWorld->GetSystemManager()->RegisterSystem<AudioVisualizerSystem>();
	mWorld->GetSystemManager()->RegisterSystem<UITransformSystem>();
	auto* uiUpdateSystem = mWorld->GetSystemManager()->RegisterSystem<UIUpdateSystem>();
	uiUpdateSystem->SetFeatures(&mUIFeatures);
	mWorld->GetSystemManager()->RegisterSystem<UIButtonSystem>();   // 버튼 마우스 히트테스트/클릭

	mWorld->GetSystemManager()->RegisterSystem<AudioSystem>();
	mWorld->GetSystemManager()->RegisterSystem<SfxSystem>();
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
		AUDIOMANAGER.InitSpectrumDSP(2048);

		Entity visEntity = mWorld->CreateEntity();
		AudioVisualizerComponent& vis = mWorld->AddComponent<AudioVisualizerComponent>(visEntity);

		// 선택: 위치/크기 커스터마이징
		vis.basePosition = Vec2(2560.f / 2, 700.f);  // 화면 하단 중앙
		vis.barWidth = 6.f;
		vis.barSpacing = 0.5f;
		vis.maxBarHeight = 25.f;
		vis.gain = 8.f;
		vis.isVisible = true;

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

	// 원형 오디오 비주얼라이저 — 화면 중앙, 흰색 방사형 막대 (미니멀 스타일)
	{
		Entity circEntity = mWorld->CreateEntity();
		CircularVisualizerComponent& circ = mWorld->AddComponent<CircularVisualizerComponent>(circEntity);
		circ.center       = Vec2(2560.f * 0.5f, 1440.f * 0.5f);  // 화면 정중앙
		circ.baseRadius   = 160.f;  // 막대 안쪽 끝 고정 반지름 — 중앙 원은 비워짐
		circ.minBarLength = 4.f;    // 무음 시 점선 링 형태 유지
		circ.maxBarLength = 90.f;
		circ.barWidth     = 3.f;    // 둘레 간격(2πr/128 ≈ 7.9px)보다 작아 막대끼리 분리됨
		circ.gain         = 12.f;

#ifdef _IMGUI
		IMGUIComponent& circImgui = mWorld->AddComponent<IMGUIComponent>(circEntity);
		std::vector<EditorProperty> circProps;
		circProps.push_back({ "Center",          PropertyType::Vec2,  &circ.center,        0.f,    0.f });
		circProps.push_back({ "Base Radius",     PropertyType::Float, &circ.baseRadius,   10.f,  600.f });
		circProps.push_back({ "Min Bar Length",  PropertyType::Float, &circ.minBarLength,  0.f,   30.f });
		circProps.push_back({ "Max Bar Length",  PropertyType::Float, &circ.maxBarLength,  5.f,  400.f });
		circProps.push_back({ "Bar Width",       PropertyType::Float, &circ.barWidth,      1.f,   10.f });
		circProps.push_back({ "Gain",            PropertyType::Float, &circ.gain,          0.1f,  30.f });
		circProps.push_back({ "Rise Smooth",     PropertyType::Float, &circ.riseSmooth,    1.f,   80.f });
		circProps.push_back({ "Fall Smooth",     PropertyType::Float, &circ.fallSmooth,    0.1f,  30.f });
		circProps.push_back({ "Use Spikes",      PropertyType::Bool,  &circ.useSpikes,     0.f,    0.f });
		circProps.push_back({ "Visible",         PropertyType::Bool,  &circ.isVisible,     0.f,    0.f });
		circImgui.RegisterEditorProperties(circProps);
		circImgui.SetName("Circular Visualizer");
#endif
	}

	// 로비 Room 엔티티
	{
		Entity roomStateEntity = mWorld->CreateEntity();
		mWorld->AddComponent<LobbyRoomStateComponent>(roomStateEntity);
		mWorld->AddComponent<LobbyRoomListComponent>(roomStateEntity);
	}

	// 방 목록 ImGui 버전
	mUIFeatures.push_back(std::make_shared<LobbyRoomBrowserFeature>());

	// 오디오 비주얼라이저 바
	mUIFeatures.push_back(std::make_shared<UIAudioVisualizerFeature>());

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

	
	AUDIOMANAGER.RequestBGM("event:/OST/Escort", SOUNDNAME::Ambient);
	//PlayerPrefab player{mWorld.get()};
	PrefabFactory::RegisterAllPrefabs();
	//TerrainPrefab terrain{ mWorld.get() };
	SkyBoxPrefab skybox{ mWorld.get() };
	DirLightPrefab light{ mWorld.get() };

	OceanPrefab ocean{ mWorld.get() };

	//EnemyPrefab	enemys {mWorld.get() };




	// MAP export json load
	// [참고] 현재 FBX LOADER에서 NormalMap을 읽지 못하게 함.
	// 
	//LoadJsonLevel(L"..\\Resources\\Json\\M_StylizedStudyLogCabin_A1_Export.json");
	// LoadJsonLevel(L"..\\Resources\\Json\\ThirdPersonMap_Export.json");
	LoadJsonLevelData(L"..\\Resources\\Json\\Map001_Export.json");
	LoadCollisionJson(L"..\\Resources\\Json\\Map001_Nav_Export.json");


	/////////////////////////////////////////////////////////////////////


#pragma region Sample
	/////////////////////////////////////////////////////////////////////
	// [ 샘플 ]

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
#pragma endregion


#pragma region UI

	CreatePauseMenu();

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


	auto portraitModule = std::make_shared<HUDPortraitUpdateFeature>();
	mUIFeatures.push_back(portraitModule);

	auto skillCooldownModule = std::make_shared<HUDSkillCooldownFeature>();
	mUIFeatures.push_back(skillCooldownModule);


	for (const auto& feature : mUIFeatures)
	{
		if (feature != nullptr)
			feature->Initialize(mWorld.get());
	}


#pragma endregion

	
#pragma region Systems
	mWorld->Initialize();

	// INPUT
	mWorld->GetSystemManager()->RegisterSystem<PlayerInputSystem>();

	// NETWORK
	mWorld->GetSystemManager()->RegisterSystem<NetRecvSystem>(mWorld->GetNetIdMap());
	mWorld->GetSystemManager()->RegisterSystem<NetSendSystem>();
	mWorld->GetSystemManager()->RegisterSystem<GamePhaseSystem>();
	mWorld->GetSystemManager()->RegisterSystem<PauseSystem>();
	mWorld->GetSystemManager()->RegisterSystem<UIButtonSystem>();
#if USE_CPU_ANIMATION
	mWorld->GetSystemManager()->RegisterSystem<CpuAnimationSystem>();
#else
	mWorld->GetSystemManager()->RegisterSystem<AnimationSystem>();
#endif
	mWorld->GetSystemManager()->RegisterSystem<CameraSystem>();

	mWorld->GetSystemManager()->RegisterSystem<EnemySystem>();
	mWorld->GetSystemManager()->RegisterSystem<PlayerSystem>();
	mWorld->GetSystemManager()->RegisterSystem<SpectateSystem>();  // Sim: 관전 대상 선정(입력=PlayerInputSystem, 프레이밍=CameraSystem)
	mWorld->GetSystemManager()->RegisterSystem<TransformSystem>();
	mWorld->GetSystemManager()->RegisterSystem<MovementSystem>();
	mWorld->GetSystemManager()->RegisterSystem<DamageFeedbackSystem>();
	mWorld->GetSystemManager()->RegisterSystem<RhythmSystem>();
	mWorld->GetSystemManager()->RegisterSystem<AudioVisualizerSystem>();
	mWorld->GetSystemManager()->RegisterSystem<UITransformSystem>();
	auto* uiUpdateSystem = mWorld->GetSystemManager()->RegisterSystem<UIUpdateSystem>();
	uiUpdateSystem->SetFeatures(&mUIFeatures);
	mWorld->GetSystemManager()->RegisterSystem<AudioSystem>();
	mWorld->GetSystemManager()->RegisterSystem<SfxSystem>();
	mWorld->GetSystemManager()->RegisterSystem<BeatSystem>();
	mWorld->GetSystemManager()->RegisterSystem<NetInterpolationSystem>();
	mWorld->GetSystemManager()->RegisterSystem<SocketSystem>();
	mWorld->GetSystemManager()->RegisterSystem<SocketFollowSystem>();
	mWorld->GetSystemManager()->RegisterSystem<WeaponTrailSystem>();
	mWorld->GetSystemManager()->RegisterSystem<AnimNotifySystem>();
	mWorld->GetSystemManager()->RegisterSystem<DashSpeedLineSystem>();
	mWorld->GetSystemManager()->RegisterSystem<VfxSystem>();
	ParticleSystem* particleSystem = mWorld->GetSystemManager()->RegisterSystem<ParticleSystem>();


	auto* renderSystemFS = mWorld->GetSystemManager()->RegisterSystem<RenderSystem>();
	renderSystemFS->SetPipeline(make_shared<GameRenderPipeline>());
	shared_ptr<GameRenderPipeline> gp = static_pointer_cast<GameRenderPipeline>(renderSystemFS->GetPipeline());
	gp->SetWorldUIFeature(&mUIFeatures);


	auto* uiRenderSystem = mWorld->GetSystemManager()->RegisterSystem<UIRenderSystem>();
	uiRenderSystem->SetFeatures(&mUIFeatures);

#pragma endregion

//	// 오디오 비주얼라이저 (확인용 — ImGui Inspector "Audio Visualizer"의 Visible로 토글)
//	{
//		AUDIOMANAGER.InitSpectrumDSP(2048);  // 이미 초기화됐으면 no-op
//
//		Entity visEntity = mWorld->CreateEntity();
//		AudioVisualizerComponent& vis = mWorld->AddComponent<AudioVisualizerComponent>(visEntity);
//		vis.basePosition = Vec2(2560.f / 2, 700.f);  // 화면 하단 중앙
//		vis.barWidth = 6.f;
//		vis.barSpacing = 0.5f;
//		vis.maxBarHeight = 25.f;
//		vis.gain = 8.f;
//
//#ifdef _IMGUI
//		IMGUIComponent& visImgui = mWorld->AddComponent<IMGUIComponent>(visEntity);
//		std::vector<EditorProperty> props;
//		props.push_back({ "Base Position",  PropertyType::Vec2,  &vis.basePosition,  0.f,    0.f });
//		props.push_back({ "Bar Width",      PropertyType::Float, &vis.barWidth,       1.f,   50.f });
//		props.push_back({ "Bar Spacing",    PropertyType::Float, &vis.barSpacing,     0.f,   20.f });
//		props.push_back({ "Max Height",     PropertyType::Float, &vis.maxBarHeight,   10.f, 800.f });
//		props.push_back({ "Gain",           PropertyType::Float, &vis.gain,           0.1f,  30.f });
//		props.push_back({ "Rise Smooth",    PropertyType::Float, &vis.riseSmooth,     1.f,   50.f });
//		props.push_back({ "Fall Smooth",    PropertyType::Float, &vis.fallSmooth,     0.1f,  20.f });
//		props.push_back({ "Visible",        PropertyType::Bool,  &vis.isVisible,      0.f,    0.f });
//		visImgui.RegisterEditorProperties(props);
//		visImgui.SetName("Audio Visualizer");
//#endif
//	}

	mWorld->AddSingleton<GameRuleComponent>();

	particleSystem->SpawnEffect(L"Particle_AuraRise", Vec3(-8002.9f, 1027.2f, -12519.6f));


}



void SecondScene::Initialize()
{
	mWorld->SetSceneId(mSceneId);

	
	AUDIOMANAGER.RequestBGM("event:/Escort", SOUNDNAME::Ambient);
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
	LoadJsonLevel(L"..\\Resources\\Json\\MapDesert_Export.json");
	LoadCollisionJson(L"..\\Resources\\Json\\MapDesert_Export.json");

	/////////////////////////////////////////////////////////////////////////



#pragma region UI

	CreatePauseMenu();

#pragma endregion

	/////////////////////////////////////////////////////////////////////////



	// HUD / 게임 UI Feature
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

	auto portraitModule = std::make_shared<HUDPortraitUpdateFeature>();
	mUIFeatures.push_back(portraitModule);

	auto skillCooldownModule = std::make_shared<HUDSkillCooldownFeature>();
	mUIFeatures.push_back(skillCooldownModule);

	for (const auto& feature : mUIFeatures)
	{
		if (feature != nullptr)
			feature->Initialize(mWorld.get());
	}


	mWorld->Initialize();

	// INPUT
	mWorld->GetSystemManager()->RegisterSystem<PlayerInputSystem>();

	// NETWORK
	mWorld->GetSystemManager()->RegisterSystem<NetRecvSystem>(mWorld->GetNetIdMap());
	mWorld->GetSystemManager()->RegisterSystem<NetSendSystem>();
	mWorld->GetSystemManager()->RegisterSystem<GamePhaseSystem>();
	mWorld->GetSystemManager()->RegisterSystem<PauseSystem>();
	mWorld->GetSystemManager()->RegisterSystem<UIButtonSystem>();
#if USE_CPU_ANIMATION
	mWorld->GetSystemManager()->RegisterSystem<CpuAnimationSystem>();
#else
	mWorld->GetSystemManager()->RegisterSystem<AnimationSystem>();
#endif
	mWorld->GetSystemManager()->RegisterSystem<CameraSystem>();

	mWorld->GetSystemManager()->RegisterSystem<EnemySystem>();
	mWorld->GetSystemManager()->RegisterSystem<PlayerSystem>();
	mWorld->GetSystemManager()->RegisterSystem<SpectateSystem>();
	mWorld->GetSystemManager()->RegisterSystem<TransformSystem>();
	mWorld->GetSystemManager()->RegisterSystem<MovementSystem>();
	mWorld->GetSystemManager()->RegisterSystem<DamageFeedbackSystem>();
	mWorld->GetSystemManager()->RegisterSystem<RhythmSystem>();
	mWorld->GetSystemManager()->RegisterSystem<AudioVisualizerSystem>();
	mWorld->GetSystemManager()->RegisterSystem<UITransformSystem>();
	auto* uiUpdateSystem = mWorld->GetSystemManager()->RegisterSystem<UIUpdateSystem>();
	uiUpdateSystem->SetFeatures(&mUIFeatures);
	mWorld->GetSystemManager()->RegisterSystem<AudioSystem>();
	mWorld->GetSystemManager()->RegisterSystem<SfxSystem>();
	mWorld->GetSystemManager()->RegisterSystem<BeatSystem>();
	mWorld->GetSystemManager()->RegisterSystem<NetInterpolationSystem>();
	mWorld->GetSystemManager()->RegisterSystem<SocketSystem>();
	mWorld->GetSystemManager()->RegisterSystem<SocketFollowSystem>();
	mWorld->GetSystemManager()->RegisterSystem<WeaponTrailSystem>();
	mWorld->GetSystemManager()->RegisterSystem<AnimNotifySystem>();
	mWorld->GetSystemManager()->RegisterSystem<DashSpeedLineSystem>();
	mWorld->GetSystemManager()->RegisterSystem<VfxSystem>();
	mWorld->GetSystemManager()->RegisterSystem<ParticleSystem>();

	auto* renderSystemSS = mWorld->GetSystemManager()->RegisterSystem<RenderSystem>();
	renderSystemSS->SetPipeline(make_shared<GameRenderPipeline>());
	shared_ptr<GameRenderPipeline> gp = static_pointer_cast<GameRenderPipeline>(renderSystemSS->GetPipeline());
	gp->SetWorldUIFeature(&mUIFeatures);

	auto* uiRenderSystem = mWorld->GetSystemManager()->RegisterSystem<UIRenderSystem>();
	uiRenderSystem->SetFeatures(&mUIFeatures);

	mWorld->AddSingleton<GameRuleComponent>();
}

void ThirdScene::Initialize()
{
	mWorld->SetSceneId(mSceneId);

	AUDIOMANAGER.RequestBGM("event:/OST/Escort", SOUNDNAME::Ambient);
	PrefabFactory::RegisterAllPrefabs();


	SkyBoxPrefab skybox{ mWorld.get() };
	DirLightPrefab light{ mWorld.get() };

#pragma region UI

	CreatePauseMenu();


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

	auto portraitModule = std::make_shared<HUDPortraitUpdateFeature>();
	mUIFeatures.push_back(portraitModule);

	auto skillCooldownModule = std::make_shared<HUDSkillCooldownFeature>();
	mUIFeatures.push_back(skillCooldownModule);

	for (const auto& feature : mUIFeatures)
	{
		if (feature != nullptr)
			feature->Initialize(mWorld.get());
	}

#pragma endregion

	mWorld->Initialize();

	// INPUT
	mWorld->GetSystemManager()->RegisterSystem<PlayerInputSystem>();

	// NETWORK
	mWorld->GetSystemManager()->RegisterSystem<NetRecvSystem>(mWorld->GetNetIdMap());
	mWorld->GetSystemManager()->RegisterSystem<NetSendSystem>();
	mWorld->GetSystemManager()->RegisterSystem<GamePhaseSystem>();
	mWorld->GetSystemManager()->RegisterSystem<PauseSystem>();
	mWorld->GetSystemManager()->RegisterSystem<UIButtonSystem>();
#if USE_CPU_ANIMATION
	mWorld->GetSystemManager()->RegisterSystem<CpuAnimationSystem>();
#else
	mWorld->GetSystemManager()->RegisterSystem<AnimationSystem>();
#endif
	mWorld->GetSystemManager()->RegisterSystem<CameraSystem>();

	mWorld->GetSystemManager()->RegisterSystem<EnemySystem>();
	mWorld->GetSystemManager()->RegisterSystem<PlayerSystem>();
	mWorld->GetSystemManager()->RegisterSystem<SpectateSystem>();
	mWorld->GetSystemManager()->RegisterSystem<TransformSystem>();
	mWorld->GetSystemManager()->RegisterSystem<MovementSystem>();
	mWorld->GetSystemManager()->RegisterSystem<DamageFeedbackSystem>();
	mWorld->GetSystemManager()->RegisterSystem<RhythmSystem>();
	mWorld->GetSystemManager()->RegisterSystem<AudioVisualizerSystem>();
	mWorld->GetSystemManager()->RegisterSystem<UITransformSystem>();
	auto* uiUpdateSystem = mWorld->GetSystemManager()->RegisterSystem<UIUpdateSystem>();
	uiUpdateSystem->SetFeatures(&mUIFeatures);
	mWorld->GetSystemManager()->RegisterSystem<AudioSystem>();
	mWorld->GetSystemManager()->RegisterSystem<SfxSystem>();
	mWorld->GetSystemManager()->RegisterSystem<BeatSystem>();
	mWorld->GetSystemManager()->RegisterSystem<NetInterpolationSystem>();
	mWorld->GetSystemManager()->RegisterSystem<SocketSystem>();
	mWorld->GetSystemManager()->RegisterSystem<SocketFollowSystem>();
	mWorld->GetSystemManager()->RegisterSystem<WeaponTrailSystem>();
	mWorld->GetSystemManager()->RegisterSystem<AnimNotifySystem>();
	mWorld->GetSystemManager()->RegisterSystem<DashSpeedLineSystem>();
	mWorld->GetSystemManager()->RegisterSystem<VfxSystem>();
	mWorld->GetSystemManager()->RegisterSystem<ParticleSystem>();

	auto* renderSystemTS = mWorld->GetSystemManager()->RegisterSystem<RenderSystem>();
	renderSystemTS->SetPipeline(make_shared<GameRenderPipeline>());
	shared_ptr<GameRenderPipeline> gp = static_pointer_cast<GameRenderPipeline>(renderSystemTS->GetPipeline());
	gp->SetWorldUIFeature(&mUIFeatures);

	auto* uiRenderSystem = mWorld->GetSystemManager()->RegisterSystem<UIRenderSystem>();
	uiRenderSystem->SetFeatures(&mUIFeatures);

	mWorld->AddSingleton<GameRuleComponent>();
}
#pragma endregion







#pragma region Victory / Lose Scene

void VictoryScene::Initialize()
{
	mWorld->SetSceneId(mSceneId);
	PrefabFactory::RegisterAllPrefabs();
	SkyBoxPrefab skybox{ mWorld.get() };
	DirLightPrefab light{ mWorld.get() };

	// 메인 카메라
	{
		Entity cam = mWorld->CreateEntity();
		TransformComponent t{};
		t.mLocalPosition = { 0.f, 0.f, -100.f };
		mWorld->AddComponent<MainCameraComponent>(cam);
		mWorld->AddComponent<CameraComponent>(cam);
		mWorld->AddComponent<TransformComponent>(cam, t);
	}

	// 풀스크린 STAGE CLEAR 배경 이미지
	{
		Entity bg = mWorld->CreateEntity();
		auto& tr = mWorld->AddComponent<UITransformComponent>(bg);
		tr.mLayoutMode   = UILayoutMode::ScreenRatio;
		tr.mAnchor       = Anchor::Center;
		tr.mPositionRatio = Vec2(0.f, 0.f);
		tr.mSizeRatio     = Vec2(1.f, 1.f);
		tr.mPivot        = Vec2(0.5f, 0.5f);
		tr.mUILayerIndex = 1;
		mWorld->AddComponent<UISpriteComponent>(bg, RESOURCEMANAGER.Get<Texture>(L"UI_StageClear_0"));
	}

	// VICTORY 타이틀 텍스트
	{
		Entity title = mWorld->CreateEntity();
		auto& tr = mWorld->AddComponent<UITransformComponent>(title);
		tr.mAnchor       = Anchor::Center;
		tr.mPosition     = Vec2(0.f, -250.f);
		tr.mSize         = Vec2(600.f, 120.f);
		tr.mPivot        = Vec2(0.5f, 0.5f);
		tr.mUILayerIndex = 5;
		auto& txt = mWorld->AddComponent<UITextComponent>(title);
		txt.mText = L"VICTORY";
	}

	// MAIN MENU 버튼
	CreateUIButton(mWorld.get(), {
		.anchor   = Anchor::Center,
		.position = Vec2(0.f, 250.f),
		.size     = Vec2(320.f, 72.f),
		.visual   = UIButtonVisual::Vfx,
		.resKey   = L"VFX_UI_Select",
		.label    = L"MAIN MENU",
		.onClick  = []() { gEngine->GetSceneManager().RequestScene(SceneId::MainMenu); },
		});

	mWorld->Initialize();

	mWorld->GetSystemManager()->RegisterSystem<TransformSystem>();
	mWorld->GetSystemManager()->RegisterSystem<CameraSystem>();
	mWorld->GetSystemManager()->RegisterSystem<UITransformSystem>();
	auto* uiUpdateSystem = mWorld->GetSystemManager()->RegisterSystem<UIUpdateSystem>();
	uiUpdateSystem->SetFeatures(&mUIFeatures);
	mWorld->GetSystemManager()->RegisterSystem<UIButtonSystem>();
	mWorld->GetSystemManager()->RegisterSystem<AudioSystem>();
	mWorld->GetSystemManager()->RegisterSystem<SfxSystem>();
	mWorld->GetSystemManager()->RegisterSystem<ParticleSystem>();

	auto* renderSystemVS = mWorld->GetSystemManager()->RegisterSystem<RenderSystem>();
	renderSystemVS->SetPipeline(make_shared<GameRenderPipeline>());

	auto* uiRenderSystem = mWorld->GetSystemManager()->RegisterSystem<UIRenderSystem>();
	uiRenderSystem->SetFeatures(&mUIFeatures);

	mSceneId = SceneId::VGame;
}

void LoseScene::Initialize()
{
	mWorld->Initialize();

}

#pragma endregion
