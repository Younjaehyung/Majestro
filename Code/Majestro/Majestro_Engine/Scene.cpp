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
#include "GpuResourceBudget.h"
#include "EngineLog.h"
#include "LobbyRoomStateComponent.h"
#include "LobbyRoomListComponent.h"
#include "LobbyRoomSystem.h"
#include "NpcComponent.h"
#include "NpcInteractionSystem.h"
#include "UIDialogueFeature.h"
#include "UILevelSelectFeature.h"
#include "UIRhythmSelectFeature.h"
#include "JsonUtils.h"
#include "LobbyRoomBrowserFeature.h"
#include "MenuRoomBrowserSystem.h"

#include "RenderSystem.h"
#include "GameRenderPipeline.h"
#include "DecalSystem.h"
#include "BuffAuraSystem.h"
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
#include "RhythmEmissiveSystem.h"
#include "HighlightSystem.h"
#include "DamagePopupUpdateFeature.h"
#include "UIEmoteFeature.h"

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
#include "UIScoreBoardFeature.h"
#include "UIResultBoardFeature.h"
#include "UIComboHudFeature.h"
#include "UltimateCutInFeature.h"
#include "PlayerStatusUIFeature.h"
#include "UIPhaseProgressUpdateFeature.h"

#include "MainMenuController.h"
#include "MainMenuCameraComponent.h"
#include "MainMenuSystem.h"
#include "MainMenuCameraSystem.h"
#include "IntroSequenceComponent.h"
#include "IntroSequenceSystem.h"
#include "AirshipDepartureComponent.h"
#include "AirshipDepartureSystem.h"
#include "CloudDriftComponent.h"
#include "CloudDriftSystem.h"
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

	struct LoadingSceneProfile
	{
		SceneId sceneId;
		const wchar_t* mapJsonPath;
		const wchar_t* backgroundTextureKey;
	};

	constexpr const wchar_t* kDefaultLoadingBackgroundKey = L"UI_Loading_Main_01";

	const LoadingSceneProfile* FindLoadingSceneProfile(SceneId id)
	{
		// 씬별 로딩 설정을 한곳에서 관리한다.
		// 새 씬의 로딩 배경이나 선로딩 맵을 추가할 때 이 테이블만 확장하면 된다.
		static constexpr std::array<LoadingSceneProfile, 5> profiles =
		{
			LoadingSceneProfile{ SceneId::Plaza, L"..\\Resources\\Json\\MapShip_Export.json", L"UI_Loading_Plaza" },
			LoadingSceneProfile{ SceneId::FirstGame, L"..\\Resources\\Json\\Map001_Export.json", L"UI_Loading_FirstGame" },
			LoadingSceneProfile{ SceneId::SecondGame, L"..\\Resources\\Json\\MapDesert_Export.json", L"UI_Loading_SecondGame" },
			LoadingSceneProfile{ SceneId::ThirdGame, L"..\\Resources\\Json\\Map003_Export.json", L"UI_Loading_ThirdGame" },
			LoadingSceneProfile{ SceneId::FourthGame, L"..\\Resources\\Json\\MapDragon_Export.json", L"UI_Loading_ThirdGame" },
		};

		for (const LoadingSceneProfile& profile : profiles)
		{
			if (profile.sceneId == id)
				return &profile;
		}

		return nullptr;
	}
}

Scene::Scene()
{

}

void Scene::Initialize()
{

}

void Scene::Release()
{
	Shudown();
	mWorld->Clear();
	mUIFeatures.clear();

	if (!mResourcePrefixes.empty())
	{
		RESOURCEMANAGER.UnloadSceneResources(mResourcePrefixes);
		mResourcePrefixes.clear();
	}
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

void Scene::TrackResourcePrefix(const wstring& prefix)
{
	if (prefix.empty())
		return;

	if (std::find(mResourcePrefixes.begin(), mResourcePrefixes.end(), prefix) == mResourcePrefixes.end())
		mResourcePrefixes.push_back(prefix);
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
	TrackResourcePrefix(prefix);
	for (const auto& fbxName : uniqueFbxNames)
	{
		RESOURCEMANAGER.LoadFBXModel(BuildMapFbxPath(level.levelName, fbxName), prefix);
	}

	RESOURCEMANAGER.ApplyMapSky(level);
}

void Scene::LoadJsonLevelData(const wstring& path,
	const std::function<bool(const std::string& fbxStem)>& skipMeshStem) {
	int i = 0;
	try
	{
		LevelImportData level = RESOURCEMANAGER.LoadMapResourceJson(path);
		const std::wstring prefix = s2ws(level.levelName);
		TrackResourcePrefix(prefix);

		RESOURCEMANAGER.ApplyMapSky(level);

		for (const auto& inst : level.instances)
		{
			// 파일명만 추출
			std::string name = filesystem::path(inst.fbx).filename().stem().string();

			// 제외 대상은 정적 임포트에서 건너뛴다.
			if (skipMeshStem && skipMeshStem(name))
				continue;

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
		TrackResourcePrefix(prefix);

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
		TrackResourcePrefix(prefix);
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

		// 캐릭터별 전체화면 배경 레이어 (dim 보다 뒤). pause 진입 시 PauseSystem 이 텍스처 교체.
		Entity pauseBg = mWorld->CreateEntity();
		{
			auto& tr = mWorld->AddComponent<UITransformComponent>(pauseBg);
			tr.mAnchor = Anchor::Center;        // 중앙 기준 줌인 연출을 위해 중앙 정렬
			tr.mPivot = Vec2(0.5f, 0.5f);       // 중앙 기준 줌
			tr.UseScreenRatioLayout(Vec2(0.f, 0.f), Vec2(1.f, 1.f));
			tr.mUILayerIndex = 101;
			// 초기 텍스처는 placeholder. 실제 캐릭터별 텍스처는 pause 진입 시 교체.
			auto& bgSp = mWorld->AddComponent<UISpriteComponent>(pauseBg,
				RESOURCEMANAGER.Get<Texture>(L"UI_Loading_Main_01"));
			bgSp.mColorTint = Vec4(1.f, 1.f, 1.f, 0.f);   // 알파 0 시작(등장 연출 전 숨김)
		}

		// 풀스크린 배경 (반투명 검정 dim — 캐릭터 배경 위에 깔려 대비 확보)
		Entity pauseDim = mWorld->CreateEntity();
		{
			auto& tr = mWorld->AddComponent<UITransformComponent>(pauseDim);
			tr.mAnchor = Anchor::TopLeft;
			tr.mPivot = Vec2(0.f, 0.f);
			tr.UseScreenRatioLayout(Vec2(0.f, 0.f), Vec2(1.f, 1.f));
			tr.mUILayerIndex = 100;            // 버튼(5)보다 뒤
			//auto& sp = mWorld->AddComponent<UISpriteComponent>(pauseDim,
			//	RESOURCEMANAGER.Get<Texture>(L"UI_Loading_Main_01"));
			//sp.mColorTint = Vec4(1.f, 1.f, 1.f, 0.0f);
		}

		// Pause: 타이틀 + Resume / Setting / Disconnect
		const wchar_t* pauseButtonSheet = L"UI_Paused_Sheet_01";
		const RECT pauseReturnSprite = { 54, 32, 718, 128 };
		const RECT pauseSettingSprite = { 818, 33, 1106, 150 };
		const RECT pauseBackTitleSprite = { 54, 195, 520, 287 };
		const RECT pauseManualSprite = { 822, 195, 1188, 287 };
		const RECT pauseHoverDefaultSprite = { 0, 366, 768, 405 };

		// Pause Main 버튼은 아틀라스의 글자 스프라이트를 사용하고 기능만 기존 콜백을 유지한다.
		Entity bResume = CreateUIButton(mWorld.get(), {
			.position = Vec2(160.f, -170.f),
			.size = Vec2(900.f, 128.f),
			.pivot = Vec2(0.f, 0.5f),
			.layer = 105,
			.visual = UIButtonVisual::Texture,
			.resKey = pauseButtonSheet,
			.sourceRect = pauseReturnSprite,
			.normalScale = 1.f,
			.hoveredScale = 1.f,
			.pressedScale = 0.98f,
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
			.position = Vec2(160.f, 0.f),
			.size = Vec2(390.f, 120.f),
			.pivot = Vec2(0.f, 0.5f),
			.layer = 105,
			.visual = UIButtonVisual::Texture,
			.resKey = pauseButtonSheet,
			.sourceRect = pauseSettingSprite,
			.normalScale = 1.f,
			.hoveredScale = 1.f,
			.pressedScale = 0.98f,
			.onClick = [requestPause]() { requestPause(PauseMenuState::SettingGraphics); },
			});
		Entity bDisconnect = CreateUIButton(mWorld.get(), {
			.position = Vec2(160.f, 255.f),
			.size = Vec2(630.f, 120.f),
			.pivot = Vec2(0.f, 0.5f),
			.layer = 105,
			.visual = UIButtonVisual::Texture,
			.resKey = pauseButtonSheet,
			.sourceRect = pauseBackTitleSprite,
			.normalScale = 1.f,
			.hoveredScale = 1.f,
			.pressedScale = 0.98f,
			.onClick = [requestPause]() { requestPause(PauseMenuState::ConfirmDisconnect); },
			});
		Entity bManualPause = CreateUIButton(mWorld.get(), {
			.position = Vec2(160.f, 85.f),
			.size = Vec2(480.f, 120.f),
			.pivot = Vec2(0.f, 0.5f),
			.layer = 105,
			.visual = UIButtonVisual::Texture,
			.resKey = pauseButtonSheet,
			.sourceRect = pauseManualSprite,
			.normalScale = 1.f,
			.hoveredScale = 1.f,
			.pressedScale = 0.98f,
			.onClick = [requestPause]() { requestPause(PauseMenuState::Manual); },
			});

		// 각 버튼의 하단에 캐릭터별 색상으로 교체되는 호버 선을 별도 상위 레이어로 배치한다.
		auto createPauseHoverLine =
			[this, pauseButtonSheet, pauseHoverDefaultSprite](const Vec2& position) -> Entity
			{
				Entity line = mWorld->CreateEntity();
				auto& tr = mWorld->AddComponent<UITransformComponent>(line);
				tr.mAnchor = Anchor::Center;
				tr.mPosition = position;
				tr.mSize = Vec2(1024.f, 52.f);
				tr.mPivot = Vec2(0.f, 0.5f);
				tr.mUILayerIndex = 106;

				auto& sprite = mWorld->AddComponent<UISpriteComponent>(
					line, RESOURCEMANAGER.Get<Texture>(pauseButtonSheet));
				sprite.SetSourceRect(
					static_cast<float>(pauseHoverDefaultSprite.left),
					static_cast<float>(pauseHoverDefaultSprite.top),
					static_cast<float>(
						pauseHoverDefaultSprite.right - pauseHoverDefaultSprite.left),
					static_cast<float>(
						pauseHoverDefaultSprite.bottom - pauseHoverDefaultSprite.top));
				sprite.mVisible = false;
				return line;
			};

		const Entity resumeHoverLine =
			createPauseHoverLine(Vec2(160.f, -104.f));
		const Entity settingHoverLinePause =
			createPauseHoverLine(Vec2(160.f, 66.f));
		const Entity manualHoverLine =
			createPauseHoverLine(Vec2(160.f, 151.f));
		const Entity disconnectHoverLine =
			createPauseHoverLine(Vec2(160.f, 321.f));

		auto bindPauseHoverLine = [this](Entity buttonEntity, Entity lineEntity)
			{
				if (auto* button =
					mWorld->GetComponent<UIButtonComponent>(buttonEntity))
				{
					button->mOnHoverEnter = [this, lineEntity]()
						{
							if (auto* sprite =
								mWorld->GetComponent<UISpriteComponent>(lineEntity))
								sprite->mVisible = true;
						};
					button->mOnHoverExit = [this, lineEntity]()
						{
							if (auto* sprite =
								mWorld->GetComponent<UISpriteComponent>(lineEntity))
								sprite->mVisible = false;
						};
				}
			};

		bindPauseHoverLine(bResume, resumeHoverLine);
		bindPauseHoverLine(bSetting, settingHoverLinePause);
		bindPauseHoverLine(bManualPause, manualHoverLine);
		bindPauseHoverLine(bDisconnect, disconnectHoverLine);

		// Setting (Graphics / Sound 탭)
		// 공유 요소: 제목 + 탭 헤더 2개 + BACK (두 탭 상태에 모두 포함)
		// Pause Setting의 탭 버튼 두 개를 화면 중앙을 기준으로 좌우 대칭 배치한다.
		const Vec2 tabSize = { 330.f, 92.f };
		Entity bTabGraphics = CreateUIButton(mWorld.get(), {
			.position = Vec2(-195.f, -300.f), .size = tabSize,
			.visual = UIButtonVisual::Texture, .resKey = L"UI_Title_Sheet_02",
			.sourceRect = RECT{ 0, 0, 768, 256 },
			.label = L"GRAPHICS",
			.onClick = [requestPause]() { requestPause(PauseMenuState::SettingGraphics); },
			});
		Entity bTabSound = CreateUIButton(mWorld.get(), {
			.position = Vec2(195.f, -300.f), .size = tabSize,
			.visual = UIButtonVisual::Texture, .resKey = L"UI_Title_Sheet_02",
			.sourceRect = RECT{ 0, 0, 768, 256 },
			.label = L"SOUND",
			.onClick = [requestPause]() { requestPause(PauseMenuState::SettingSound); },
			});
		Entity bSettingBack = CreateUIButton(mWorld.get(), {
			.position = Vec2(850.f, 670.f),
			.size = Vec2(182.f, 69.f),
			.layer = 106,
			.visual = UIButtonVisual::Texture,
			.resKey = L"UI_Title_Sheet_01",
			.sourceRect = RECT{ 1347, 34, 1529, 103 },
			.onClick = [requestPause]() { requestPause(PauseMenuState::Root); },
			.clickSfxKey = "ui/back",
			});

		// Graphics 탭
		std::vector<Entity> gfxCheckEntities;
		auto makeGfxToggle =
			[this, &gfxCheckEntities](Vec2 pos, const wchar_t* name, std::function<bool&()> field) -> Entity
			{
				Entity btn = CreateUIButton(mWorld.get(), {
					.position = pos, .size = Vec2(350.f, 92.f),
					.visual = UIButtonVisual::Texture, .resKey = L"UI_Title_Sheet_02",
					.sourceRect = RECT{ 0, 0, 768, 256 },
					.label = name,
					});
				Entity check = CreateUIButton(mWorld.get(), {
					.position = pos + Vec2(245.f, 0.f),
					.size = Vec2(88.f, 88.f),
					.visual = UIButtonVisual::Texture,
					.resKey = L"UI_Title_Sheet_02",
					.sourceRect = RECT{ 768, 0, 1024, 256 },
					});
				auto refresh = [this, check, field]()
					{
						if (auto* sprite = mWorld->GetComponent<UISpriteComponent>(check))
						{
							const RECT source = field()
								? RECT{ 1024, 0, 1280, 256 }
								: RECT{ 768, 0, 1024, 256 };
							sprite->SetSourceRect(
								static_cast<float>(source.left),
								static_cast<float>(source.top),
								static_cast<float>(source.right - source.left),
								static_cast<float>(source.bottom - source.top));
						}
					};
				auto toggle = [this, field, refresh]()
					{
						field() = !field();
						if (auto* rs = mWorld->GetSystemManager()->GetSystem<RenderSystem>())
							if (auto p = std::static_pointer_cast<GameRenderPipeline>(rs->GetPipeline()))
								p->ApplyGraphicsSettings();
						refresh();
					};
				if (auto* b = mWorld->GetComponent<UIButtonComponent>(btn))
					b->mOnClick = toggle;
				if (auto* b = mWorld->GetComponent<UIButtonComponent>(check))
					b->mOnClick = toggle;
				refresh();
				gfxCheckEntities.push_back(check);
				return btn;
			};

		// 이름 상자와 체크박스를 포함한 두 열 전체의 중심이 화면 중앙에 오도록 이동한다.
		Entity gFog     = makeGfxToggle({ -307.f, -150.f }, L"FOG",     []() -> bool& { return RENDERMANAGER.GetGraphicsSettings().bFog; });
		Entity gGodRay  = makeGfxToggle({ 193.f, -150.f }, L"GODRAY",  []() -> bool& { return RENDERMANAGER.GetGraphicsSettings().bGodRay; });
		Entity gBloom   = makeGfxToggle({ -307.f, -20.f }, L"BLOOM",   []() -> bool& { return RENDERMANAGER.GetGraphicsSettings().bBloom; });
		Entity gOutline = makeGfxToggle({ 193.f, -20.f }, L"OUTLINE", []() -> bool& { return RENDERMANAGER.GetGraphicsSettings().bOutline; });
		Entity gHBAO    = makeGfxToggle({ -307.f, 110.f }, L"HBAO",    []() -> bool& { return RENDERMANAGER.GetGraphicsSettings().bHBAO; });
		Entity gFXAA    = makeGfxToggle({ 193.f, 110.f }, L"FXAA",    []() -> bool& { return RENDERMANAGER.GetGraphicsSettings().bFXAA; });
		Entity gShadow  = makeGfxToggle({ -307.f, 240.f }, L"SHADOW",  []() -> bool& { return RENDERMANAGER.GetGraphicsSettings().bShadow; });

		// Sound 탭: Master / VFX / Ambient 음량 (좌우 화살표 10% 스텝)
		std::vector<Entity> soundEntities;
		auto makeVolumeRow =
			[this, &soundEntities](float rowY, const wchar_t* name, AudioCategory cat)
			{
				// 라벨
				Entity label = mWorld->CreateEntity();
				{
					auto& tr = mWorld->AddComponent<UITransformComponent>(label);
					tr.mAnchor = Anchor::Center;
					tr.mPosition = Vec2(-280.f, rowY);
					tr.mSize = Vec2(340.f, 100.f);
					tr.mPivot = Vec2(0.5f, 0.5f);
					tr.mUILayerIndex = 105;
					auto& sprite = mWorld->AddComponent<UISpriteComponent>(
						label, RESOURCEMANAGER.Get<Texture>(L"UI_Title_Sheet_02"));
					sprite.SetSourceRect(0.f, 0.f, 768.f, 256.f);
					mWorld->AddComponent<UITextComponent>(label).mText = name;
				}
				// 값 텍스트
				Entity valueText = mWorld->CreateEntity();
				{
					auto& tr = mWorld->AddComponent<UITransformComponent>(valueText);
					tr.mAnchor = Anchor::Center;
					tr.mPosition = Vec2(180.f, rowY);
					tr.mSize = Vec2(180.f, 90.f);
					tr.mPivot = Vec2(0.5f, 0.5f);
					tr.mUILayerIndex = 105;
					auto& sprite = mWorld->AddComponent<UISpriteComponent>(
						valueText, RESOURCEMANAGER.Get<Texture>(L"UI_Title_Sheet_02"));
					sprite.SetSourceRect(0.f, 0.f, 768.f, 256.f);
					mWorld->AddComponent<UITextComponent>(valueText);
				}
				auto refresh = [this, valueText, cat]()
					{
						if (auto* t = mWorld->GetComponent<UITextComponent>(valueText))
						{
							int pct = (int)(AUDIOMANAGER.GetCategoryVolume(cat) * 100.f + 0.5f); // 0~1 → 반올림 %
							// 메인 메뉴 Sound와 동일하게 숫자 값만 표시한다.
							t->mText = std::to_wstring(pct);
						}
					};
				auto step = [this, cat, refresh](float delta)
					{
						AUDIOMANAGER.SetCategoryVolume(cat, AUDIOMANAGER.GetCategoryVolume(cat) + delta); // 내부 clamp
						refresh();
					};
				Entity minus = CreateUIButton(mWorld.get(), {
					.position = Vec2(-20.f, rowY), .size = Vec2(150.f, 84.f),
					.visual = UIButtonVisual::Texture, .resKey = L"UI_Title_Sheet_02",
					.sourceRect = RECT{ 0, 0, 768, 256 },
					.label = L"<",
					.onClick = [step]() { step(-0.1f); },
					});
				Entity plus = CreateUIButton(mWorld.get(), {
					.position = Vec2(380.f, rowY), .size = Vec2(150.f, 84.f),
					.visual = UIButtonVisual::Texture, .resKey = L"UI_Title_Sheet_02",
					.sourceRect = RECT{ 0, 0, 768, 256 },
					.label = L">",
					.onClick = [step]() { step(+0.1f); },
					});
				refresh();
				soundEntities.insert(soundEntities.end(), { label, valueText, minus, plus });
			};
		makeVolumeRow(-130.f, L"MASTER",  AudioCategory::Master);
		makeVolumeRow(  30.f, L"VFX",     AudioCategory::Vfx);
		makeVolumeRow( 190.f, L"AMBIENT", AudioCategory::Ambient);

		// ConfirmDisconnect: Yes / No
		Entity bYes = CreateUIButton(mWorld.get(), {
			.position = Vec2(-180.f, 80.f),
			.size = Vec2(106.f, 74.f),
			.layer = 106,
			.visual = UIButtonVisual::Texture,
			.resKey = L"UI_Title_Sheet_01",
			.sourceRect = RECT{ 730, 156, 836, 230 },
			.onClick = [this]()
			{
				Network::GetInstance().Shutdown();
				mGameMode->mTargetSceneId = SceneId::MainMenu;
				mGameMode->IsSceneChanging() = true;
			},
			});
		Entity bNo = CreateUIButton(mWorld.get(), {
			.position = Vec2(180.f, 80.f),
			.size = Vec2(139.f, 76.f),
			.layer = 106,
			.visual = UIButtonVisual::Texture,
			.resKey = L"UI_Title_Sheet_01",
			.sourceRect = RECT{ 408, 154, 547, 230 },
			.onClick = [requestPause]() { requestPause(PauseMenuState::Root); },
			.clickSfxKey = "ui/back",
			});

		// 상태별 entity 등록 (Hidden 은 비움). 배경(캐릭터 레이어 + dim)은 모든 상태 공유.
		auto* pctrl = mWorld->GetComponent<PauseMenuController>(pauseCtrlEnt);
		pctrl->mBackgroundEntity = pauseBg;
		pctrl->mRootButtons = { bResume, bManualPause, bSetting, bDisconnect };
		pctrl->mRootHoverLines = {
			resumeHoverLine,
			manualHoverLine,
			settingHoverLinePause,
			disconnectHoverLine
		};

		// Pause의 모든 하위 화면에서 공통으로 보이는 상단과 하단 문구 띠를 만든다.
		// 각 가장자리에 두 장을 이어 배치하고 PauseSystem에서 위치를 순환시킨다.
		auto createPauseWordBand =
			[this](const Vec2& position) -> Entity
			{
				Entity band = mWorld->CreateEntity();
				auto& transform = mWorld->AddComponent<UITransformComponent>(band);
				transform.mAnchor = Anchor::TopLeft;
				transform.mPivot = Vec2(0.0f, 0.0f);
				transform.mPosition = position;
				// 기준 해상도 너비에 맞춰 한 장이 화면 전체를 덮도록 표시한다.
				transform.mSize = Vec2(2560.0f, 170.0f);
				transform.mUILayerIndex = 107;

				auto& sprite = mWorld->AddComponent<UISpriteComponent>(
					band, RESOURCEMANAGER.Get<Texture>(L"UI_Paused_Word_Sheet"));
				sprite.SetSourceRect(0.0f, 0.0f, 2048.0f, 128.0f);
				sprite.mVisible = false;

				mWorld->AddComponent<UIRenderGroupComponent>(
					band, UIRenderGroup::Pause);
				return band;
			};

		pctrl->mWordBandEntities = {
			createPauseWordBand(Vec2(0.0f, pctrl->mWordBandTopY)),
			createPauseWordBand(Vec2(
				pctrl->mWordBandTileWidth, pctrl->mWordBandTopY)),
			createPauseWordBand(Vec2(0.0f, pctrl->mWordBandBottomY)),
			createPauseWordBand(Vec2(
				pctrl->mWordBandTileWidth, pctrl->mWordBandBottomY))
		};

		pctrl->mStateEntities[(size_t)PauseMenuState::Root] = {
			pauseBg,
			pauseDim,
			bResume,
			bSetting,
			bManualPause,
			bDisconnect
		};

		// 두 Setting 탭이 공유하는 요소 (제목/탭헤더/BACK + 배경)
		// Manual 화면은 캐릭터별 안내 배경과 우측 하단 Back 버튼만 사용한다.
		pctrl->mStateEntities[(size_t)PauseMenuState::Manual] = {
			pauseBg,
			pauseDim,
			bSettingBack
		};

		const std::vector<Entity> settingShared = {
			pauseBg,
			pauseDim,
			bTabGraphics,
			bTabSound,
			bSettingBack
		};

		// Graphics 탭
		std::vector<Entity> gfxState = settingShared;
		gfxState.insert(gfxState.end(), { gFog, gGodRay, gBloom, gOutline, gHBAO, gFXAA, gShadow });
		gfxState.insert(gfxState.end(), gfxCheckEntities.begin(), gfxCheckEntities.end());
		pctrl->mStateEntities[(size_t)PauseMenuState::SettingGraphics] = std::move(gfxState);

		// Sound 탭 
		std::vector<Entity> sndState = settingShared;
		sndState.insert(sndState.end(), soundEntities.begin(), soundEntities.end());
		pctrl->mStateEntities[(size_t)PauseMenuState::SettingSound] = std::move(sndState);

		pctrl->mStateEntities[(size_t)PauseMenuState::ConfirmDisconnect] = {
			pauseBg,
			pauseDim,
			bYes,
			bNo
		};

		// 시작 시 모든 일시정지 UI 숨김
		// Hidden을 제외한 모든 Pause 화면에서 동일한 문구 띠를 공유한다.
		for (int s = static_cast<int>(PauseMenuState::Root);
			s < static_cast<int>(PauseMenuState::Count); ++s)
		{
			pctrl->mStateEntities[s].insert(
				pctrl->mStateEntities[s].end(),
				pctrl->mWordBandEntities.begin(),
				pctrl->mWordBandEntities.end());
		}

		// Pause 관련 엔티티를 하나의 렌더 그룹으로 묶는다.
		// Pause가 활성화되면 이 그룹만 표시되어 인게임 HUD가 앞에 나오지 않는다.
		for (int s = 0; s < static_cast<int>(PauseMenuState::Count); ++s)
		{
			for (Entity entity : pctrl->mStateEntities[s])
			{
				if (!mWorld->HasComponent<UIRenderGroupComponent>(entity))
				mWorld->AddComponent<UIRenderGroupComponent>(
					entity, UIRenderGroup::Pause);
			}
		}
		for (Entity entity : pctrl->mRootHoverLines)
		{
			if (entity != NULL_ENTITY &&
				!mWorld->HasComponent<UIRenderGroupComponent>(entity))
			{
				mWorld->AddComponent<UIRenderGroupComponent>(
					entity, UIRenderGroup::Pause);
			}
		}

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
		mLoadingBackground = mWorld->CreateEntity();
		auto& tr = mWorld->AddComponent<UITransformComponent>(mLoadingBackground);
		tr.mAnchor = Anchor::TopLeft;
		tr.mPivot = Vec2(0.f, 0.f);
		tr.UseScreenRatioLayout(Vec2(0.f, 0.f), Vec2(1.f, 1.f));

		shared_ptr<Texture> loadingMaterial = RESOURCEMANAGER.Get<Texture>(L"UI_Loading_Main_01");


		mWorld->AddComponent<UISpriteComponent>(mLoadingBackground, loadingMaterial);
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
		shared_ptr<Texture> progressBarMaterial = RESOURCEMANAGER.Get<Texture>(L"Loading");
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
}

void LoadingScene::Render()
{
	mWorld->Render();
}

void LoadingScene::ApplyLoadingBackground(SceneId id)
{
	const LoadingSceneProfile* profile = FindLoadingSceneProfile(id);
	const wchar_t* textureKey = profile ? profile->backgroundTextureKey : kDefaultLoadingBackgroundKey;

	shared_ptr<Texture> texture = RESOURCEMANAGER.Get<Texture>(textureKey);
	if (!texture)
		texture = RESOURCEMANAGER.Get<Texture>(kDefaultLoadingBackgroundKey);

	if (UISpriteComponent* sprite = mWorld->GetComponent<UISpriteComponent>(mLoadingBackground))
	{
				sprite->mTexture = texture;
	}
}

bool LoadingScene::LoadScene(SceneId id)
{
	mTargetSceneId = id;
	// 이전 로딩이 중단되거나 새 명령으로 덮인 경우를 대비해 작업 큐를 초기화한다.
	while (!mLoadTasks.empty())
		mLoadTasks.pop();
	while (!mLoadTaskLabels.empty())
		mLoadTaskLabels.pop();
	mTotalTaskCount = 0;

	ApplyLoadingBackground(id);

	const LoadingSceneProfile* profile = FindLoadingSceneProfile(id);
	if (!profile)
		return false;

	LevelImportData level = RESOURCEMANAGER.LoadMapResourceJson(profile->mapJsonPath);

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
		mLoadTaskLabels.push(fbxName);
		mLoadTasks.push([fbxName, levelName]() {
			shared_ptr<FBXData> data = RESOURCEMANAGER.LoadFBXModel(BuildMapFbxPath(levelName, fbxName), s2ws(levelName));});
	}

	mTotalTaskCount = (int32)mLoadTasks.size();


	// 인게임 리듬 트랙(Elec/Bass/Drum)은 전역 BGM 슬롯이라 씬 전환만으로는 멈추지 않는다.
	// 로딩 진입 시 명시적으로 정지해 로딩 음악 위에 이전 게임 BGM이 겹쳐 재생되는 것을 막는다.
	// (다음 게임 씬 AudioSystem::Initialize가 fresh 인스턴스로 다시 요청·T0 정렬)
	AUDIOMANAGER.StopBGM(SOUNDNAME::Elec);
	AUDIOMANAGER.StopBGM(SOUNDNAME::Bass);
	AUDIOMANAGER.StopBGM(SOUNDNAME::Drum);
	AUDIOMANAGER.RequestBGM("event:/OST/Loading", SOUNDNAME::Ambient);
	AUDIOMANAGER.Update(0.f);

	return true;
}

void LoadingScene::ProcessTask()
{
	if (!mLoadTasks.empty())
	{
		std::string label = "unknown";
		if (!mLoadTaskLabels.empty())
			label = mLoadTaskLabels.front();

		if (EngineLog::Enabled(EngineLog::Domain::LoadingTask))
			EngineLog::Prefix(EngineLog::Domain::LoadingTask, "begin")
				<< "task=" << label << "\n";

		std::string beforeLabel = "loading-before-" + label;
		GpuResourceBudget::DumpDxgi(beforeLabel.c_str(),
			RENDERMANAGER.GetDevice()->GetAdapter().Get(),
			RENDERMANAGER.GetGraphicsMemory().get());

		auto task = mLoadTasks.front();
		task(); // 작업 실행
		mLoadTasks.pop();


		if (!mLoadTaskLabels.empty())
			mLoadTaskLabels.pop();

		std::string afterLabel = "loading-after-" + label;

		GpuResourceBudget::DumpDxgi(afterLabel.c_str(),
			RENDERMANAGER.GetDevice()->GetAdapter().Get(),
			RENDERMANAGER.GetGraphicsMemory().get());

		if (EngineLog::Enabled(EngineLog::Domain::LoadingTask))
			EngineLog::Prefix(EngineLog::Domain::LoadingTask, "end")
				<< "task=" << label << "\n";
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
		transform.mLocalPosition = { 200.f, 612.f, -4270.f };
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
		std::vector<Cinematic::CameraView> views = RESOURCEMANAGER.LoadCameraViews(
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
		const wchar_t* titleButtonSheet = L"UI_Title_Sheet_01";
		const float startX = 850.f;   // 화면 중앙 기준 X 오프셋 (위쪽 버튼)
		const float startY = 270.f;   // 화면 중앙 기준 Y 오프셋 (위쪽 버튼)
		const float gap = 100.f;

		// 각 아틀라스 셀에서 실제로 보이는 픽셀 영역만 사용한다.
		// 기존 버튼 중심 위치를 유지하면서 투명 영역이 클릭 범위에 포함되는 것을 막는다.
		const RECT gameStartSprite = { 986, 38, 1249, 90 };
		const RECT controlSprite   = { 709, 38, 884, 90 };
		const RECT settingSprite   = { 398, 39, 552, 102 };
		const RECT exitGameSprite  = { 47, 38, 271, 90 };
		const RECT backSprite      = { 1347, 34, 1529, 103 };
		const RECT nextSprite      = { 65, 168, 248, 228 };
		const RECT yesSprite       = { 730, 156, 836, 230 };
		const RECT noSprite        = { 408, 154, 547, 230 };
		const RECT hoverLineSprite = { 1628, 51, 1887, 67 };

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
			.size = Vec2(263.f, 52.f),
			.visual = UIButtonVisual::Texture,
			.resKey = titleButtonSheet,
			.sourceRect = gameStartSprite,
			.onClick = [this, requestState]()
			{
				requestState(MainMenuState::RoomList);	// RoomList 상태로 전환
				Network::GetInstance().Awake();			// 서버 접속
			},
			});
		Entity bManual = CreateUIButton(mWorld.get(), {
			.position = Vec2(startX, startY + gap * 1),
			.size = Vec2(175.f, 52.f),
			.visual = UIButtonVisual::Texture,
			.resKey = titleButtonSheet,
			.sourceRect = controlSprite,
			.onClick = [requestState]() { requestState(MainMenuState::Manual); },
			});
		Entity bSetting = CreateUIButton(mWorld.get(), {
			.position = Vec2(startX, startY + gap * 2),
			.size = Vec2(154.f, 63.f),
			.visual = UIButtonVisual::Texture,
			.resKey = titleButtonSheet,
			.sourceRect = settingSprite,
			.onClick = [requestState]() { requestState(MainMenuState::Setting); },
			});
		Entity bMainExit = CreateUIButton(mWorld.get(), {
			.position = Vec2(startX, startY + gap * 3),
			.size = Vec2(224.f, 52.f),
			.visual = UIButtonVisual::Texture,
			.resKey = titleButtonSheet,
			.sourceRect = exitGameSprite,
			.onClick = [requestState]() { requestState(MainMenuState::Exit); },
			});

		// 메인 메뉴 버튼 위에 표시할 호버 강조선을 별도 상위 레이어로 만든다.
		// 강조선은 버튼과 같은 기준 좌표를 사용하고 버튼 하단에 겹쳐 표시한다.
		auto createHoverLine = [this, titleButtonSheet, hoverLineSprite](const Vec2& buttonPosition) -> Entity
			{
				Entity line = mWorld->CreateEntity();
				auto& tr = mWorld->AddComponent<UITransformComponent>(line);
				tr.mAnchor = Anchor::Center;
				tr.mPosition = buttonPosition + Vec2(0.f, 36.f);
				tr.mSize = Vec2(259.f, 16.f);
				tr.mPivot = Vec2(0.5f, 0.5f);
				tr.mUILayerIndex = 6;

				auto& sp = mWorld->AddComponent<UISpriteComponent>(
					line, RESOURCEMANAGER.Get<Texture>(titleButtonSheet));
				sp.SetSourceRect(
					static_cast<float>(hoverLineSprite.left),
					static_cast<float>(hoverLineSprite.top),
					static_cast<float>(hoverLineSprite.right - hoverLineSprite.left),
					static_cast<float>(hoverLineSprite.bottom - hoverLineSprite.top));
				sp.mVisible = false;
				return line;
			};

		const Entity gameStartHoverLine = createHoverLine(Vec2(startX, startY + gap * 0));
		const Entity controlHoverLine   = createHoverLine(Vec2(startX, startY + gap * 1));
		const Entity settingHoverLine   = createHoverLine(Vec2(startX, startY + gap * 2));
		const Entity exitHoverLine      = createHoverLine(Vec2(startX, startY + gap * 3));

		// 버튼의 기존 클릭 기능은 유지하고 호버 진입과 이탈 시 강조선 표시만 전환한다.
		auto bindHoverLine = [this](Entity button, Entity line)
			{
				if (auto* btn = mWorld->GetComponent<UIButtonComponent>(button))
				{
					btn->mOnHoverEnter = [this, line]()
						{
							if (auto* sp = mWorld->GetComponent<UISpriteComponent>(line))
								sp->mVisible = true;
						};
					btn->mOnHoverExit = [this, line]()
						{
							if (auto* sp = mWorld->GetComponent<UISpriteComponent>(line))
								sp->mVisible = false;
						};
				}
			};

		bindHoverLine(bGameStart, gameStartHoverLine);
		bindHoverLine(bManual, controlHoverLine);
		bindHoverLine(bSetting, settingHoverLine);
		bindHoverLine(bMainExit, exitHoverLine);

		// 서브 화면(Setting/Manual/RoomList) 공유 Back/Exit
		Entity bBack = CreateUIButton(mWorld.get(), {
			.position = Vec2(startX, startY + gap * 4),
			.size = Vec2(182.f, 69.f),
			.visual = UIButtonVisual::Texture,
			.resKey = titleButtonSheet,
			.sourceRect = backSprite,
			.onClick = [requestState]() { requestState(MainMenuState::MainMenu); },
			.clickSfxKey = "ui/back",
			});

		// Control 화면의 현재 페이지를 0부터 3까지 순환시키기 위한 인덱스다.
		// Back 버튼의 기존 위치는 유지하고 Next 버튼을 같은 높이의 오른쪽에 배치한다.
		auto controlPageIndex = std::make_shared<size_t>(0);
		Entity bControlNext = CreateUIButton(mWorld.get(), {
			.position = Vec2(startX + 250.f, startY + gap * 4),
			.size = Vec2(183.f, 60.f),
			.visual = UIButtonVisual::Texture,
			.resKey = titleButtonSheet,
			.sourceRect = nextSprite,
			.onClick = [this, ctrlEnt, controlPageIndex]()
			{
				static const std::array<const wchar_t*, 4> controlPageKeys = {
					L"UI_Title_Control_0",
					L"UI_Title_Control_1",
					L"UI_Title_Control_2",
					L"UI_Title_Control_3"
				};

				*controlPageIndex = (*controlPageIndex + 1) % controlPageKeys.size();

				// Manual 상태가 사용하는 배경 엔티티의 텍스처만 교체해 카메라와 버튼 상태는 유지한다.
				if (auto* controller = mWorld->GetComponent<MainMenuController>(ctrlEnt))
				{
					const Entity background =
						controller->mStates[(size_t)MainMenuState::Manual].background;
					if (auto* sprite = mWorld->GetComponent<UISpriteComponent>(background))
						sprite->mTexture =
							RESOURCEMANAGER.Get<Texture>(controlPageKeys[*controlPageIndex]);
				}
			},
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
		// Setting 화면은 상자, 빈 체크박스, 선택 체크박스의 세 아틀라스 영역을 사용한다.
		const wchar_t* settingSheet = L"UI_Title_Sheet_02";
		const RECT settingBoxSprite = { 0, 0, 768, 256 };
		const RECT settingCheckOffSprite = { 768, 0, 1024, 256 };
		const RECT settingCheckOnSprite = { 1024, 0, 1280, 256 };

		auto settingGraphicsEntities = std::make_shared<std::vector<Entity>>();
		auto settingSoundEntities = std::make_shared<std::vector<Entity>>();
		auto settingSoundTab = std::make_shared<bool>(false);

		// 상자 스프라이트와 글꼴을 조합한 읽기 전용 항목을 생성한다.
		auto createSettingLabel =
			[this, settingSheet, settingBoxSprite](
				const Vec2& position, const Vec2& size, const wchar_t* text) -> Entity
			{
				Entity entity = mWorld->CreateEntity();
				auto& tr = mWorld->AddComponent<UITransformComponent>(entity);
				tr.mAnchor = Anchor::Center;
				tr.mPosition = position;
				tr.mSize = size;
				tr.mPivot = Vec2(0.5f, 0.5f);
				tr.mUILayerIndex = 5;

				auto& sprite = mWorld->AddComponent<UISpriteComponent>(
					entity, RESOURCEMANAGER.Get<Texture>(settingSheet));
				sprite.SetSourceRect(
					static_cast<float>(settingBoxSprite.left),
					static_cast<float>(settingBoxSprite.top),
					static_cast<float>(settingBoxSprite.right - settingBoxSprite.left),
					static_cast<float>(settingBoxSprite.bottom - settingBoxSprite.top));

				auto& label = mWorld->AddComponent<UITextComponent>(entity);
				label.mText = text;
				return entity;
			};

		auto setSettingEntityVisible = [this](Entity entity, bool visible)
			{
				if (auto* sprite = mWorld->GetComponent<UISpriteComponent>(entity))
					sprite->mVisible = visible;
				if (auto* text = mWorld->GetComponent<UITextComponent>(entity))
					text->mVisible = visible;
				if (auto* button = mWorld->GetComponent<UIButtonComponent>(entity))
				{
					if (!visible)
					{
						button->mHovered = false;
						button->mPressed = false;
					}
					button->mEnabled = visible;
				}
			};

		// Graphics와 Sound 탭도 같은 상자 스프라이트와 글꼴 조합으로 만든다.
		Entity bSettingGraphicsTab = CreateUIButton(mWorld.get(), {
			.position = Vec2(80.f, -300.f),
			.size = Vec2(330.f, 92.f),
			.visual = UIButtonVisual::Texture,
			.resKey = settingSheet,
			.sourceRect = settingBoxSprite,
			.label = L"GRAPHICS",
			});
		Entity bSettingSoundTab = CreateUIButton(mWorld.get(), {
			.position = Vec2(470.f, -300.f),
			.size = Vec2(330.f, 92.f),
			.visual = UIButtonVisual::Texture,
			.resKey = settingSheet,
			.sourceRect = settingBoxSprite,
			.label = L"SOUND",
			});

		// 이름 상자와 체크박스 모두 같은 토글 기능을 수행한다.
		auto createGraphicsToggle =
			[this, settingSheet, settingBoxSprite, settingCheckOffSprite,
			 settingCheckOnSprite, settingGraphicsEntities](
				const Vec2& position, const wchar_t* name,
				std::function<bool&()> field)
			{
				Entity nameButton = CreateUIButton(mWorld.get(), {
					.position = position,
					.size = Vec2(350.f, 92.f),
					.visual = UIButtonVisual::Texture,
					.resKey = settingSheet,
					.sourceRect = settingBoxSprite,
					.label = name,
					});

				Entity checkButton = CreateUIButton(mWorld.get(), {
					.position = position + Vec2(245.f, 0.f),
					.size = Vec2(88.f, 88.f),
					.visual = UIButtonVisual::Texture,
					.resKey = settingSheet,
					.sourceRect = settingCheckOffSprite,
					});

				auto refresh = [this, checkButton, field,
					settingCheckOffSprite, settingCheckOnSprite]()
					{
						if (auto* sprite = mWorld->GetComponent<UISpriteComponent>(checkButton))
						{
							const RECT& source =
								field() ? settingCheckOnSprite : settingCheckOffSprite;
							sprite->SetSourceRect(
								static_cast<float>(source.left),
								static_cast<float>(source.top),
								static_cast<float>(source.right - source.left),
								static_cast<float>(source.bottom - source.top));
						}
					};

				auto toggle = [this, field, refresh]()
					{
						field() = !field();
						if (auto* renderSystem =
							mWorld->GetSystemManager()->GetSystem<RenderSystem>())
						{
							// 메인 메뉴는 LobbyRenderPipeline을 사용하므로 GameRenderPipeline일 때만 즉시 반영한다.
							if (auto pipeline = std::dynamic_pointer_cast<GameRenderPipeline>(
								renderSystem->GetPipeline()))
								pipeline->ApplyGraphicsSettings();
						}
						refresh();
					};

				if (auto* button = mWorld->GetComponent<UIButtonComponent>(nameButton))
					button->mOnClick = toggle;
				if (auto* button = mWorld->GetComponent<UIButtonComponent>(checkButton))
					button->mOnClick = toggle;

				refresh();
				settingGraphicsEntities->insert(
					settingGraphicsEntities->end(), { nameButton, checkButton });
			};

		createGraphicsToggle(
			Vec2(20.f, -150.f), L"FOG",
			[]() -> bool& { return RENDERMANAGER.GetGraphicsSettings().bFog; });
		createGraphicsToggle(
			Vec2(520.f, -150.f), L"GODRAY",
			[]() -> bool& { return RENDERMANAGER.GetGraphicsSettings().bGodRay; });
		createGraphicsToggle(
			Vec2(20.f, -20.f), L"BLOOM",
			[]() -> bool& { return RENDERMANAGER.GetGraphicsSettings().bBloom; });
		createGraphicsToggle(
			Vec2(520.f, -20.f), L"OUTLINE",
			[]() -> bool& { return RENDERMANAGER.GetGraphicsSettings().bOutline; });
		createGraphicsToggle(
			Vec2(20.f, 110.f), L"HBAO",
			[]() -> bool& { return RENDERMANAGER.GetGraphicsSettings().bHBAO; });
		createGraphicsToggle(
			Vec2(520.f, 110.f), L"FXAA",
			[]() -> bool& { return RENDERMANAGER.GetGraphicsSettings().bFXAA; });
		createGraphicsToggle(
			Vec2(20.f, 240.f), L"SHADOW",
			[]() -> bool& { return RENDERMANAGER.GetGraphicsSettings().bShadow; });

		// Sound 탭은 항목 상자, 감소 버튼, 현재 값, 증가 버튼 순서로 정렬한다.
		auto createSoundRow =
			[this, settingSheet, settingBoxSprite, settingSoundEntities,
			 createSettingLabel](float rowY, const wchar_t* name, AudioCategory category)
			{
				Entity nameLabel =
					createSettingLabel(Vec2(80.f, rowY), Vec2(340.f, 100.f), name);
				Entity lowerButton = CreateUIButton(mWorld.get(), {
					.position = Vec2(340.f, rowY),
					.size = Vec2(150.f, 84.f),
					.visual = UIButtonVisual::Texture,
					.resKey = settingSheet,
					.sourceRect = settingBoxSprite,
					.label = L"<",
					});
				Entity valueLabel =
					createSettingLabel(Vec2(540.f, rowY), Vec2(180.f, 90.f), L"");
				Entity raiseButton = CreateUIButton(mWorld.get(), {
					.position = Vec2(740.f, rowY),
					.size = Vec2(150.f, 84.f),
					.visual = UIButtonVisual::Texture,
					.resKey = settingSheet,
					.sourceRect = settingBoxSprite,
					.label = L">",
					});

				auto refresh = [this, valueLabel, category]()
					{
						if (auto* text = mWorld->GetComponent<UITextComponent>(valueLabel))
						{
							const int percent = static_cast<int>(
								AUDIOMANAGER.GetCategoryVolume(category) * 100.f + 0.5f);
							text->mText = std::to_wstring(percent);
						}
					};
				auto changeVolume = [category, refresh](float amount)
					{
						AUDIOMANAGER.SetCategoryVolume(
							category,
							AUDIOMANAGER.GetCategoryVolume(category) + amount);
						refresh();
					};

				if (auto* button = mWorld->GetComponent<UIButtonComponent>(lowerButton))
					button->mOnClick = [changeVolume]() { changeVolume(-0.1f); };
				if (auto* button = mWorld->GetComponent<UIButtonComponent>(raiseButton))
					button->mOnClick = [changeVolume]() { changeVolume(0.1f); };

				refresh();
				settingSoundEntities->insert(
					settingSoundEntities->end(),
					{ nameLabel, lowerButton, valueLabel, raiseButton });
			};

		createSoundRow(-130.f, L"MASTER", AudioCategory::Master);
		createSoundRow(30.f, L"VFX", AudioCategory::Vfx);
		createSoundRow(190.f, L"AMBIENT", AudioCategory::Ambient);

		auto applySettingTab =
			[settingSoundTab, settingGraphicsEntities, settingSoundEntities,
			 setSettingEntityVisible]()
			{
				for (Entity entity : *settingGraphicsEntities)
					setSettingEntityVisible(entity, !*settingSoundTab);
				for (Entity entity : *settingSoundEntities)
					setSettingEntityVisible(entity, *settingSoundTab);
			};

		if (auto* button = mWorld->GetComponent<UIButtonComponent>(bSettingGraphicsTab))
			button->mOnClick = [settingSoundTab, applySettingTab]()
				{
					*settingSoundTab = false;
					applySettingTab();
				};
		if (auto* button = mWorld->GetComponent<UIButtonComponent>(bSettingSoundTab))
			button->mOnClick = [settingSoundTab, applySettingTab]()
				{
					*settingSoundTab = true;
					applySettingTab();
				};

		// Setting 상태가 표시된 직후 현재 선택된 내부 탭의 항목만 보이도록 다시 적용한다.
		Entity settingTabController = mWorld->CreateEntity();
		mWorld->AddComponent<UIScriptComponent>(settingTabController).mOnUpdate =
			[this, ctrlEnt, applySettingTab](float)
			{
				if (auto* controller = mWorld->GetComponent<MainMenuController>(ctrlEnt))
					if (controller->mState == MainMenuState::Setting &&
						!controller->mEntitiesPendingReveal)
						applySettingTab();
			};

		Entity bYes = CreateUIButton(mWorld.get(), {
			.position = Vec2(-200.f, 0.f),
			.size = Vec2(106.f, 74.f),
			.visual = UIButtonVisual::Texture,
			.resKey = titleButtonSheet,
			.sourceRect = yesSprite,
			.onClick = []() { PostQuitMessage(0); },
			});
		Entity bNo = CreateUIButton(mWorld.get(), {
			.position = Vec2(200.f, 0.f),
			.size = Vec2(139.f, 76.f),
			.visual = UIButtonVisual::Texture,
			.resKey = titleButtonSheet,
			.sourceRect = noSprite,
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
		std::vector<Entity> mainSettingEntities = {
			bBack,
			bSubExit,
			bSettingGraphicsTab,
			bSettingSoundTab,
			settingTabController
		};
		mainSettingEntities.insert(
			mainSettingEntities.end(),
			settingGraphicsEntities->begin(),
			settingGraphicsEntities->end());
		mainSettingEntities.insert(
			mainSettingEntities.end(),
			settingSoundEntities->begin(),
			settingSoundEntities->end());
		ctrl->mStates[(size_t)MainMenuState::Setting].entities =
			std::move(mainSettingEntities);
		// Control 화면 우측 하단에는 Back과 Next 두 버튼만 표시한다.
		ctrl->mStates[(size_t)MainMenuState::Manual].entities = { bBack, bControlNext };
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
		ctrl->mStates[(size_t)MainMenuState::Manual].background   = makeBg(L"UI_Title_Control_0");
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
				tr.mAnchor       = Anchor::TopRight;
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
	shared_ptr<GameRenderPipeline> pipelineMM = std::dynamic_pointer_cast<GameRenderPipeline>(renderSystemMM->GetPipeline());
	pipelineMM->SetMotionBlurEnabled(false);
	pipelineMM->SetColorLUT(L"ColorLUT/Scene/Clouseau 54_strip", 33);  // MAP Title LUT

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
	mWorld->GetSystemManager()->RegisterSystem<DecalSystem>();  // 데칼 수명 관리 (Sim)
	mWorld->GetSystemManager()->RegisterSystem<BuffAuraSystem>();  // 버프 오라 데칼 (Sim)
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



//
//	{
//		AUDIOMANAGER.InitSpectrumDSP(2048);
//
//		Entity visEntity = mWorld->CreateEntity();
//		AudioVisualizerComponent& vis = mWorld->AddComponent<AudioVisualizerComponent>(visEntity);
//
//		// 선택: 위치/크기 커스터마이징
//		vis.basePosition = Vec2(2560.f / 2, 700.f);  // 화면 하단 중앙
//		vis.barWidth = 6.f;
//		vis.barSpacing = 0.5f;
//		vis.maxBarHeight = 25.f;
//		vis.gain = 8.f;
//		vis.isVisible = true;
//
//#ifdef _IMGUI
//
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
//
//	// 원형 오디오 비주얼라이저 — 화면 중앙, 흰색 방사형 막대 (미니멀 스타일)
//	{
//		Entity circEntity = mWorld->CreateEntity();
//		CircularVisualizerComponent& circ = mWorld->AddComponent<CircularVisualizerComponent>(circEntity);
//		circ.center       = Vec2(2560.f * 0.5f, 1440.f * 0.5f);  // 화면 정중앙
//		circ.baseRadius   = 160.f;  // 막대 안쪽 끝 고정 반지름 — 중앙 원은 비워짐
//		circ.minBarLength = 4.f;    // 무음 시 점선 링 형태 유지
//		circ.maxBarLength = 90.f;
//		circ.barWidth     = 3.f;    // 둘레 간격(2πr/128 ≈ 7.9px)보다 작아 막대끼리 분리됨
//		circ.gain         = 12.f;
//
//#ifdef _IMGUI
//		IMGUIComponent& circImgui = mWorld->AddComponent<IMGUIComponent>(circEntity);
//		std::vector<EditorProperty> circProps;
//		circProps.push_back({ "Center",          PropertyType::Vec2,  &circ.center,        0.f,    0.f });
//		circProps.push_back({ "Base Radius",     PropertyType::Float, &circ.baseRadius,   10.f,  600.f });
//		circProps.push_back({ "Min Bar Length",  PropertyType::Float, &circ.minBarLength,  0.f,   30.f });
//		circProps.push_back({ "Max Bar Length",  PropertyType::Float, &circ.maxBarLength,  5.f,  400.f });
//		circProps.push_back({ "Bar Width",       PropertyType::Float, &circ.barWidth,      1.f,   10.f });
//		circProps.push_back({ "Gain",            PropertyType::Float, &circ.gain,          0.1f,  30.f });
//		circProps.push_back({ "Rise Smooth",     PropertyType::Float, &circ.riseSmooth,    1.f,   80.f });
//		circProps.push_back({ "Fall Smooth",     PropertyType::Float, &circ.fallSmooth,    0.1f,  30.f });
//		circProps.push_back({ "Use Spikes",      PropertyType::Bool,  &circ.useSpikes,     0.f,    0.f });
//		circProps.push_back({ "Visible",         PropertyType::Bool,  &circ.isVisible,     0.f,    0.f });
//		circImgui.RegisterEditorProperties(circProps);
//		circImgui.SetName("Circular Visualizer");
//#endif
//	}

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

	
	AUDIOMANAGER.RequestBGM("event:/OST/EscortMulti", SOUNDNAME::Ambient);
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
	LoadCollisionJson(L"..\\Resources\\Json\\Map001_Export.json");


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

	auto emoteModule = std::make_shared<UIEmoteFeature>();
	mUIFeatures.push_back(emoteModule);

	auto gameInfoModule = std::make_shared<UIGameInfoUpdateFeature>();
	mUIFeatures.push_back(gameInfoModule);

	// 점수판
	auto resultBoardModule = std::make_shared<UIResultBoardFeature>();
	mUIFeatures.push_back(resultBoardModule);


	auto ultimateCutInModule = std::make_shared<UltimateCutInFeature>();
	mUIFeatures.push_back(ultimateCutInModule);

	// 콤보 랭크
	auto comboHudModule = std::make_shared<UIComboHudFeature>();
	mUIFeatures.push_back(comboHudModule);

	auto playerStatusModule = std::make_shared<PlayerStatusUIFeature>();
	mUIFeatures.push_back(playerStatusModule);

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
	mWorld->GetSystemManager()->RegisterSystem<DecalSystem>();  // 데칼 수명 관리 (Sim)
	mWorld->GetSystemManager()->RegisterSystem<BuffAuraSystem>();  // 버프 오라 데칼 (Sim)
	mWorld->GetSystemManager()->RegisterSystem<DamageFeedbackSystem>();
	mWorld->GetSystemManager()->RegisterSystem<RhythmEmissiveSystem>();
	mWorld->GetSystemManager()->RegisterSystem<HighlightSystem>();
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
	gp->SetColorLUT(L"ColorLUT/Scene/Milo 5_strip", 33);

	// FirstScene 컬러 그레이딩
	{
		ColorGradingParams cg;
		cg.Saturation = 1.3f;
		cg.Contrast   = 1.0f;
		cg.Brightness = 0.0f;
		cg.Exposure   = 1.6f;
		gp->SetColorGrading(cg);
	}

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


namespace
{
	NpcRole ParseNpcRole(const std::string& name)
	{
		if (name == "levelSelect")  return NpcRole::LevelSelect;
		if (name == "traitSelect")  return NpcRole::TraitSelect;
		if (name == "rhythmSelect") return NpcRole::RhythmSelect;
		return NpcRole::Dialogue;
	}

	// 광장 NPC 배치/대화 스크립트 로드.
	void LoadPlazaNpcs(World* world, const std::wstring& path)
	{
		std::ifstream ifs(path);
		if (!ifs.is_open())
		{
			std::cout << "[Plaza] NPC json 을 열 수 없어 NPC 스폰을 건너뜁니다" << std::endl;
			return;
		}

		json root;
		ifs >> root;
		if (!root.contains("npcs"))
			return;

		for (const auto& it : root["npcs"])
		{
			Entity e = world->CreateEntity();

			TransformComponent t{};
			t.mLocalPosition = ParseVec3ArrayOrObject(RequireJson(it, "position"), 1.0f);
			const float homeYaw = GetOptionalFloat(it, "rotationY", 0.f);
			t.mLocalRotationE = Vec3(0.f, homeYaw, 0.f);
			const float scale = GetOptionalFloat(it, "scale", 1.f);
			t.mLocalScale = Vec3(scale, scale, scale);
			world->AddComponent<TransformComponent>(e, t);

			NpcComponent& npc = world->AddComponent<NpcComponent>(e);
			npc.mNpcId = utfs2ws(GetOptionalString(it, "id", "npc"));
			npc.mName = utfs2ws(GetOptionalString(it, "name", "NPC"));
			npc.mRole = ParseNpcRole(GetOptionalString(it, "role", "dialogue"));
			npc.mPortraitKey = utfs2ws(GetOptionalString(it, "portrait", "UI_NoteMan_Portrait"));
			// 아틀라스 초상화 셀(없으면 전체 텍스처 사용)
			if (it.contains("portraitRect") && it["portraitRect"].is_array() && it["portraitRect"].size() >= 4)
			{
				const auto& r = it["portraitRect"];
				npc.mPortraitRect = Vec4(r[0].get<float>(), r[1].get<float>(), r[2].get<float>(), r[3].get<float>());
			}
			npc.mInteractRadius = GetOptionalFloat(it, "interactRadius", 250.f);
			npc.mHomeYaw = homeYaw;

			if (it.contains("dialogue"))
				for (const auto& lineJson : it["dialogue"])
					npc.mDialogueLines.push_back(utfs2ws(GetOptionalString(lineJson, "line", "")));

			// 표시용 메시. 지정 리소스가 없으면 Cube 폴백, 그래도 없으면 비표시로 스폰.
			shared_ptr<Mesh> mesh = RESOURCEMANAGER.Get<Mesh>(utfs2ws(GetOptionalString(it, "mesh", "SM_Noteman")));
			if (!mesh) mesh = RESOURCEMANAGER.Get<Mesh>(L"Cube");
			shared_ptr<Material> material = RESOURCEMANAGER.Get<Material>(utfs2ws(GetOptionalString(it, "material", "Anim_NoteMan_Idle0")));
			if (mesh && material)
			{
				std::vector<shared_ptr<Material>> materials{ material };
				world->AddComponent<RenderComponent>(e, mesh, materials);

				shared_ptr<Animator> idleAnim = RESOURCEMANAGER.Get<Animator>(utfs2ws(GetOptionalString(it, "anim", "Anim_NoteMan_Idle")));
				if (idleAnim)
				{
					std::vector<shared_ptr<Animator>> animators{ idleAnim };
					world->AddComponent<AnimationComponent>(e, animators);
				}
			}
			else
			{
				std::cout << "[Plaza] NPC 표시 리소스(mesh/material) 없음 — 비표시로 스폰: "
					<< GetOptionalString(it, "id", "npc") << std::endl;
			}
		}
	}

	
	bool IsPlazaCloudMesh(const std::string& fbxStem)
	{
		return fbxStem.rfind("SM_SM_Cloud", 0) == 0;
	}

	void SpawnPlazaClouds(World* world, const std::wstring& path)
	{
		LevelImportData level = RESOURCEMANAGER.LoadMapResourceJson(path);
		const std::wstring prefix = s2ws(level.levelName);

		constexpr float kCloudDistanceScale = 0.3f;

		int spawned = 0;
		for (const auto& inst : level.instances)
		{
			const std::string stem = filesystem::path(inst.fbx).filename().stem().string();
			if (!IsPlazaCloudMesh(stem))
				continue;
			
			shared_ptr<FBXData> data = RESOURCEMANAGER.Get<FBXData>(ResourceManager::MakeKey(prefix, s2ws(stem)));
			if (!data || data->GetMeshs().empty())
				continue;


			const Matrix& M = inst.worldMtx;
			Vec3 row0(M._11, M._12, M._13);
			Vec3 row1(M._21, M._22, M._23);
			Vec3 row2(M._31, M._32, M._33);
			float sx = row0.Length(); if (sx < 1e-5f) sx = 1.f;
			float sy = row1.Length(); if (sy < 1e-5f) sy = 1.f;
			float sz = row2.Length(); if (sz < 1e-5f) sz = 1.f;


			Matrix R = Matrix::Identity;
			R._11 = row0.x / sx;  R._12 = row0.y / sx;  R._13 = row0.z / sx;
			R._21 = row1.x / sy;  R._22 = row1.y / sy;  R._23 = row1.z / sy;
			R._31 = -row2.x / sz; R._32 = -row2.y / sz; R._33 = -row2.z / sz;
			const Quaternion q = Quaternion::CreateFromRotationMatrix(R);
			const Vec3 er = q.ToEuler();
			const Vec3 rotDeg(XMConvertToDegrees(er.x),
							  XMConvertToDegrees(er.y),
							  XMConvertToDegrees(er.z));
			const Vec3 scale(sx, sy, sz); 


			const Vec3 translation = M.Translation() * kCloudDistanceScale;


			const float r01a = rand() / static_cast<float>(RAND_MAX);
			const float r01c = rand() / static_cast<float>(RAND_MAX);
			const float speedScale = 0.8f + r01a * 0.4f;                          // 0.8~1.2
			const float travelled  = r01c * CloudDriftSystem::kRecycleDist;       

			const auto& meshes   = data->GetMeshs();
			const auto& meshMats = data->GetMeshMaterials();
			for (size_t mi = 0; mi < meshes.size(); ++mi)
			{
				if (!meshes[mi]) continue;

				Entity e = world->CreateEntity();

				TransformComponent transform{};
				transform.mLocalPosition  = translation;
				transform.mLocalRotationE = rotDeg;
				transform.mLocalScale     = scale;
				transform.mIsStatic       = false;   // 움직여야 하므로 정적 아님
				world->AddComponent<TransformComponent>(e, transform);

				RenderComponent& render = world->AddComponent<RenderComponent>(e);
				if (mi < meshMats.size())
					render.mMaterials = meshMats[mi];
				render.SetMesh(meshes[mi]);

				CloudDriftComponent& cloud = world->AddComponent<CloudDriftComponent>(e);
				cloud.mBasePos    = translation;
				cloud.mSpeedScale = speedScale;
				cloud.mTravelled  = travelled;
			}
			spawned++;
		}
		std::cout << "[Plaza] 떠다니는 구름 메시 스폰: " << spawned << " 개" << std::endl;
	}
}

void PlazaScene::Initialize()
{
	mWorld->SetSceneId(mSceneId);

	AUDIOMANAGER.RequestBGM("event:/OST/EscortMulti", SOUNDNAME::Ambient);
	PrefabFactory::RegisterAllPrefabs();
	SkyBoxPrefab skybox{ mWorld.get() };
	DirLightPrefab light{ mWorld.get() };

	// OceanPrefab ocean{ mWorld.get() };


	// Far cloud
	{
		Entity cloudEntity = mWorld->CreateEntity();
		TransformComponent cloudTransform{};
		cloudTransform.mLocalPosition = Vec3(-2664.0f, 1371.0f, 20.0f);
		cloudTransform.mWorldPosition = cloudTransform.mLocalPosition;
		cloudTransform.mWorldMatrix = Matrix::CreateTranslation(cloudTransform.mLocalPosition);
		mWorld->AddComponent<TransformComponent>(cloudEntity, cloudTransform);

		ParticleComponent& cloudParticle = mWorld->AddComponent<ParticleComponent>(cloudEntity);
		cloudParticle.mEffectName = L"Particle_CloudDriftFar";
	}
	{
		Entity cloudEntity = mWorld->CreateEntity();
		TransformComponent cloudTransform{};
		cloudTransform.mLocalPosition = Vec3(2664.0f, 1371.0f, 20.0f);
		cloudTransform.mWorldPosition = cloudTransform.mLocalPosition;
		cloudTransform.mWorldMatrix = Matrix::CreateTranslation(cloudTransform.mLocalPosition);
		mWorld->AddComponent<TransformComponent>(cloudEntity, cloudTransform);

		ParticleComponent& cloudParticle = mWorld->AddComponent<ParticleComponent>(cloudEntity);
		cloudParticle.mEffectName = L"Particle_CloudDriftFar";
	}

	// Near cloud 
	{

		Entity cloudEntity = mWorld->CreateEntity();
		TransformComponent cloudTransform{};
		cloudTransform.mLocalPosition = Vec3(-1600.0f, 1280.0f, 20.0f);
		cloudTransform.mWorldPosition = cloudTransform.mLocalPosition;
		cloudTransform.mWorldMatrix = Matrix::CreateTranslation(cloudTransform.mLocalPosition);
		mWorld->AddComponent<TransformComponent>(cloudEntity, cloudTransform);

		ParticleComponent& cloudParticle = mWorld->AddComponent<ParticleComponent>(cloudEntity);
		cloudParticle.mEffectName = L"Particle_CloudDriftNear";
	}
	{
		Entity cloudEntity = mWorld->CreateEntity();
		TransformComponent cloudTransform{};
		cloudTransform.mLocalPosition = Vec3(1600.0f, 1280.0f, 20.0f);
		cloudTransform.mWorldPosition = cloudTransform.mLocalPosition;
		cloudTransform.mWorldMatrix = Matrix::CreateTranslation(cloudTransform.mLocalPosition);
		mWorld->AddComponent<TransformComponent>(cloudEntity, cloudTransform);

		ParticleComponent& cloudParticle = mWorld->AddComponent<ParticleComponent>(cloudEntity);
		cloudParticle.mEffectName = L"Particle_CloudDriftNear";
	}


	// 구름 메시 제외
	LoadJsonLevelData(L"..\\Resources\\Json\\MapShip_Export.json", &IsPlazaCloudMesh);
	LoadCollisionJson(L"..\\Resources\\Json\\MapShip_Export.json");

	// 구름
	SpawnPlazaClouds(mWorld.get(), L"..\\Resources\\Json\\MapShip_Export.json");

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

	auto emoteModule = std::make_shared<UIEmoteFeature>();
	mUIFeatures.push_back(emoteModule);

	auto playerStatusModule = std::make_shared<PlayerStatusUIFeature>();
	mUIFeatures.push_back(playerStatusModule);

	auto gameProgressModule = std::make_shared<UIPhaseProgressUpdateFeature>();
	mUIFeatures.push_back(gameProgressModule);

	auto portraitModule = std::make_shared<HUDPortraitUpdateFeature>();
	mUIFeatures.push_back(portraitModule);

	auto skillCooldownModule = std::make_shared<HUDSkillCooldownFeature>();
	mUIFeatures.push_back(skillCooldownModule);

	auto dialogueModule = std::make_shared<UIDialogueFeature>();	// NPC 대화 UI
	mUIFeatures.push_back(dialogueModule);

	auto levelSelectModule = std::make_shared<UILevelSelectFeature>();	// 관문지기 레벨 선택 UI
	mUIFeatures.push_back(levelSelectModule);


	auto rhythmSelectModule = std::make_shared<UIRhythmSelectFeature>();		// 음향사 파생음악 선택 UI
	mUIFeatures.push_back(rhythmSelectModule);

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
	mWorld->GetSystemManager()->RegisterSystem<NpcInteractionSystem>();	// NPC 근접/대화 (광장 전용)

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
	mWorld->GetSystemManager()->RegisterSystem<DecalSystem>();  // 데칼 수명 관리 (Sim)
	mWorld->GetSystemManager()->RegisterSystem<BuffAuraSystem>();  // 버프 오라 데칼 (Sim)
	mWorld->GetSystemManager()->RegisterSystem<DamageFeedbackSystem>();
	mWorld->GetSystemManager()->RegisterSystem<RhythmEmissiveSystem>();
	mWorld->GetSystemManager()->RegisterSystem<HighlightSystem>();
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


	auto* renderSystemPZ = mWorld->GetSystemManager()->RegisterSystem<RenderSystem>();
	renderSystemPZ->SetPipeline(make_shared<GameRenderPipeline>());
	shared_ptr<GameRenderPipeline> gp = static_pointer_cast<GameRenderPipeline>(renderSystemPZ->GetPipeline());
	gp->SetWorldUIFeature(&mUIFeatures);
	gp->SetColorLUT(L"ColorLUT/Scene/Cobi 3_strip", 33);

	// 광장 컬러 그레이딩 (FirstScene 과 동일)
	{
		ColorGradingParams cg;
		cg.Saturation = 1.3f;
		cg.Contrast   = 1.0f;
		cg.Brightness = 0.0f;
		cg.Exposure   = 1.6f;
		gp->SetColorGrading(cg);
	}

	auto* uiRenderSystem = mWorld->GetSystemManager()->RegisterSystem<UIRenderSystem>();
	uiRenderSystem->SetFeatures(&mUIFeatures);

#pragma endregion

	mWorld->AddSingleton<GameRuleComponent>();
	mWorld->AddSingleton<DialogueStateComponent>();


	mWorld->GetSystemManager()->RegisterSystem<AirshipDepartureSystem>();		// 비행정 출발 시네마틱
	mWorld->GetSystemManager()->RegisterSystem<CloudDriftSystem>();	// 구름


	auto& departure = mWorld->AddSingleton<AirshipDepartureComponent>();
	departure.mKeys = RESOURCEMANAGER.LoadCameraSequence(L"..\\Resources\\Json\\ShipDepartureCamera.json");

	// NPC 배치 + 대화 스크립트
	LoadPlazaNpcs(mWorld.get(), L"..\\Resources\\Json\\Plaza_NPCs.json");
}



void SecondScene::Initialize()
{
	mWorld->SetSceneId(mSceneId);

	
	AUDIOMANAGER.RequestBGM("event:/OST/EscortMulti", SOUNDNAME::Ambient);
	//PlayerPrefab player{mWorld.get()};
	PrefabFactory::RegisterAllPrefabs();
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

	auto emoteModule = std::make_shared<UIEmoteFeature>();
	mUIFeatures.push_back(emoteModule);

	auto gameInfoModule = std::make_shared<UIGameInfoUpdateFeature>();
	mUIFeatures.push_back(gameInfoModule);

	auto resultBoardModule = std::make_shared<UIResultBoardFeature>();
	mUIFeatures.push_back(resultBoardModule);


	auto ultimateCutInModule = std::make_shared<UltimateCutInFeature>();
	mUIFeatures.push_back(ultimateCutInModule);

	auto comboHudModule = std::make_shared<UIComboHudFeature>();
	mUIFeatures.push_back(comboHudModule);

	auto playerStatusModule = std::make_shared<PlayerStatusUIFeature>();
	mUIFeatures.push_back(playerStatusModule);

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
	mWorld->GetSystemManager()->RegisterSystem<DecalSystem>();  // 데칼 수명 관리 (Sim)
	mWorld->GetSystemManager()->RegisterSystem<BuffAuraSystem>();  // 버프 오라 데칼 (Sim)
	mWorld->GetSystemManager()->RegisterSystem<DamageFeedbackSystem>();
	mWorld->GetSystemManager()->RegisterSystem<RhythmEmissiveSystem>();
	mWorld->GetSystemManager()->RegisterSystem<HighlightSystem>();
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
	gp->SetColorLUT(L"ColorLUT/Scene/T_P6", 16);

	// SecondScene 컬러 그레이딩
	{
		ColorGradingParams cg;
		cg.Saturation = 1.5f;
		cg.Contrast   = 1.0f;
		cg.Brightness = 0.0f;
		cg.Exposure   = 1.3f;
		gp->SetColorGrading(cg);
	}

	auto* uiRenderSystem = mWorld->GetSystemManager()->RegisterSystem<UIRenderSystem>();
	uiRenderSystem->SetFeatures(&mUIFeatures);

	mWorld->AddSingleton<GameRuleComponent>();
}

void ThirdScene::Initialize()
{
	mWorld->SetSceneId(mSceneId);

	AUDIOMANAGER.RequestBGM("event:/OST/EscortMulti", SOUNDNAME::Ambient);
	PrefabFactory::RegisterAllPrefabs();


	SkyBoxPrefab skybox{ mWorld.get() };
	DirLightPrefab light{ mWorld.get() };

#pragma region UI

	CreatePauseMenu();

	LoadJsonLevelFBX(L"..\\Resources\\Json\\Map003_Export.json");
	LoadJsonLevelData(L"..\\Resources\\Json\\Map003_Export.json");


	auto audioVisualizerModule = std::make_shared<UIAudioVisualizerFeature>();
	mUIFeatures.push_back(audioVisualizerModule);

	auto actionModule = std::make_shared<UIActionUpdateFeature>();
	mUIFeatures.push_back(actionModule);

	auto hpBarModule = std::make_shared<UIHpBarUpdateFeature>();
	mUIFeatures.push_back(hpBarModule);

	auto damagePopupModule = std::make_shared<DamagePopupUpdateFeature>();
	mUIFeatures.push_back(damagePopupModule);

	auto emoteModule = std::make_shared<UIEmoteFeature>();
	mUIFeatures.push_back(emoteModule);

	auto gameInfoModule = std::make_shared<UIGameInfoUpdateFeature>();
	mUIFeatures.push_back(gameInfoModule);

	auto resultBoardModule = std::make_shared<UIResultBoardFeature>();
	mUIFeatures.push_back(resultBoardModule);

	
	auto ultimateCutInModule = std::make_shared<UltimateCutInFeature>();
	mUIFeatures.push_back(ultimateCutInModule);

	auto comboHudModule = std::make_shared<UIComboHudFeature>();
	mUIFeatures.push_back(comboHudModule);

	auto playerStatusModule = std::make_shared<PlayerStatusUIFeature>();
	mUIFeatures.push_back(playerStatusModule);

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
	mWorld->GetSystemManager()->RegisterSystem<DecalSystem>();  // 데칼 수명 관리 (Sim)
	mWorld->GetSystemManager()->RegisterSystem<BuffAuraSystem>();  // 버프 오라 데칼 (Sim)
	mWorld->GetSystemManager()->RegisterSystem<DamageFeedbackSystem>();
	mWorld->GetSystemManager()->RegisterSystem<RhythmEmissiveSystem>();
	mWorld->GetSystemManager()->RegisterSystem<HighlightSystem>();
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
	gp->SetColorLUT(L"ColorLUT/Scene/Remy 24_strip", 33);

	// ThirdScene 컬러 그레이딩
	{
		ColorGradingParams cg;
		cg.Saturation     = 1.3f;
		cg.Contrast       = 1.1f;
		cg.Brightness     = 0.04f;
		cg.Exposure       = 0.7f;
		gp->SetColorGrading(cg);
	}

	auto* uiRenderSystem = mWorld->GetSystemManager()->RegisterSystem<UIRenderSystem>();
	uiRenderSystem->SetFeatures(&mUIFeatures);

	mWorld->AddSingleton<GameRuleComponent>();

	// 씬 진입(PreparePhase 대기) 시네마틱 카메라.
	// CameraSystem 이후에 실행되어 카메라를 최종 오버라이드한다.
	mWorld->GetSystemManager()->RegisterSystem<IntroSequenceSystem>();

	// 시퀀스 데이터 로드(골격: JSON 미준비 시 빈 시퀀스 → 재생 안 함).
	// 추후 언리얼 시퀀서 export(JSON) 경로를 여기에 연결한다.
	auto& introSeq = mWorld->AddSingleton<IntroSequenceComponent>();
	introSeq.mKeys = RESOURCEMANAGER.LoadCameraSequence(
		L"..\\Resources\\Json\\Map003Camera.json");
}


void FourthScene::Initialize()
{
	mWorld->SetSceneId(mSceneId);

	AUDIOMANAGER.RequestBGM("event:/OST/EscortMulti", SOUNDNAME::Ambient);
	PrefabFactory::RegisterAllPrefabs();


	SkyBoxPrefab skybox{ mWorld.get() };
	DirLightPrefab light{ mWorld.get() };

#pragma region UI

	CreatePauseMenu();


	LoadJsonLevelFBX(L"..\\Resources\\Json\\MapDragon_Export.json");
	LoadJsonLevelData(L"..\\Resources\\Json\\MapDragon_Export.json");


	auto audioVisualizerModule = std::make_shared<UIAudioVisualizerFeature>();
	mUIFeatures.push_back(audioVisualizerModule);

	auto actionModule = std::make_shared<UIActionUpdateFeature>();
	mUIFeatures.push_back(actionModule);

	auto hpBarModule = std::make_shared<UIHpBarUpdateFeature>();
	mUIFeatures.push_back(hpBarModule);

	auto damagePopupModule = std::make_shared<DamagePopupUpdateFeature>();
	mUIFeatures.push_back(damagePopupModule);

	auto emoteModule = std::make_shared<UIEmoteFeature>();
	mUIFeatures.push_back(emoteModule);

	auto gameInfoModule = std::make_shared<UIGameInfoUpdateFeature>();
	mUIFeatures.push_back(gameInfoModule);

	auto resultBoardModule = std::make_shared<UIResultBoardFeature>();
	mUIFeatures.push_back(resultBoardModule);


	auto ultimateCutInModule = std::make_shared<UltimateCutInFeature>();
	mUIFeatures.push_back(ultimateCutInModule);

	auto comboHudModule = std::make_shared<UIComboHudFeature>();
	mUIFeatures.push_back(comboHudModule);

	auto playerStatusModule = std::make_shared<PlayerStatusUIFeature>();
	mUIFeatures.push_back(playerStatusModule);

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
	mWorld->GetSystemManager()->RegisterSystem<DecalSystem>();  // 데칼 수명 관리 (Sim)
	mWorld->GetSystemManager()->RegisterSystem<BuffAuraSystem>();  // 버프 오라 데칼 (Sim)
	mWorld->GetSystemManager()->RegisterSystem<DamageFeedbackSystem>();
	mWorld->GetSystemManager()->RegisterSystem<RhythmEmissiveSystem>();
	mWorld->GetSystemManager()->RegisterSystem<HighlightSystem>();
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
	gp->SetColorLUT(L"ColorLUT/Scene/Remy 24_strip", 33);

	// FourthScene 컬러 그레이딩 (ThirdScene 값 재사용)
	{
		ColorGradingParams cg;
		cg.Saturation     = 1.3f;
		cg.Contrast       = 1.1f;
		cg.Brightness     = 0.04f;
		cg.Exposure       = 0.7f;
		gp->SetColorGrading(cg);
	}

	auto* uiRenderSystem = mWorld->GetSystemManager()->RegisterSystem<UIRenderSystem>();
	uiRenderSystem->SetFeatures(&mUIFeatures);

	mWorld->AddSingleton<GameRuleComponent>();

	// 씬 진입(PreparePhase 대기) 시네마틱 카메라.
	mWorld->GetSystemManager()->RegisterSystem<IntroSequenceSystem>();

	auto& introSeq = mWorld->AddSingleton<IntroSequenceComponent>();
	introSeq.mKeys = RESOURCEMANAGER.LoadCameraSequence(
		L"..\\Resources\\Json\\Map003Camera.json");
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
