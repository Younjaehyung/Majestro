#include "pch.h"
#include "Scene.h"
#include "GameCore.h"
#include "ResourceManager.h"
#include "FBX.h"	
#include "World.h"
#include "Component.h"
#include "TransformComponent.h"
#include "CameraComponent.h"
#include "LightComponent.h"
#include "TagComponent.h"
#include "PlayerComponent.h"
#include "ColliderComponent.h"
#include "BeatComponent.h"
#include "GravityComponent.h"
#include "MovementComponent.h"
#include "GameRuleComponent.h"
#include "PlayerSpawnComponent.h"
#include "PathLoadComponent.h"
#include "LevelImport.h"
#include "Mesh.h"
#include "Prefab.h"

#include "GameMode.h"
#include "GamePhase.h"
#include "SystemManager.h"
#include "CameraSystem.h"
#include "TransformSystem.h"
#include "PlayerSystem.h"
#include "EnemySystem.h"
#include "BeatSystem.h"
#include "MovementSystem.h"
#include "DeathSystem.h"
#include "NetRecvSystem.h"
#include "NetSendSystem.h"
#include "PlayerInputSystem.h"
#include "BulletFireEventSystem.h"
#include "MeleeAttackSystem.h"
#include "CollisionSystem.h"
#include "DamageSystem.h"
#include "PlayerNavValidationSystem.h"
#include "BuffSystem.h"
#include "InteractionSystem.h"
#include "InteractableComponent.h"
#include "NetEntityComponent.h"
#include "EnemyComponent.h"
#include "SpawnerSystem.h"
#include "SpawnerComponent.h"
#include "EventManager.h"
#include "GameEvents.h"
#include "JsonUtils.h"
#include "GameRuleSystem.h"
#include "PathFollowSystem.h"
#include "GameTimer.h"
#include "FlyComponent.h"

namespace
{
	bool HasMeshSidecar(const std::filesystem::path& assetPath)
	{
		std::filesystem::path meshPath = assetPath.parent_path() / assetPath.stem();
		meshPath.replace_extension(".mesh");
		return std::filesystem::exists(meshPath);
	}

	std::wstring ResolveCollisionFbxPath(const std::string& jsonFbxPath)
	{
		if (jsonFbxPath.empty())
			return L"";

		const std::filesystem::path rawPath = std::filesystem::path(jsonFbxPath);
		std::vector<std::filesystem::path> candidates;

		if (rawPath.is_absolute())
			candidates.push_back(rawPath);

		candidates.push_back(std::filesystem::path("..") / "Resources" / rawPath);
		candidates.push_back(std::filesystem::path("..") / "Resources" / "FBX" / rawPath.filename());

		std::filesystem::path upperExt = rawPath.filename();
		upperExt.replace_extension(".FBX");
		candidates.push_back(std::filesystem::path("..") / "Resources" / "FBX" / upperExt);

		for (const std::filesystem::path& candidate : candidates)
		{
			if (std::filesystem::exists(candidate) || HasMeshSidecar(candidate))
				return candidate.wstring();
		}

		return candidates.empty() ? L"" : candidates.back().wstring();
	}

	// 맵 FBX 경로
	std::wstring BuildLevelFbxPath(const std::string& levelName, const std::string& stem)
	{
		if (!levelName.empty())
		{
			std::string subBase = "..\\Resources\\FBX\\" + levelName + "\\" + stem;
			if (std::filesystem::exists(subBase + ".mesh"))	// Resources\FBX\<level>\<stem>.mesh 가 있으면 서브폴더
				return s2ws(subBase + ".fbx");
		}

		// 없으면 기존 flat 경로
		return s2ws("..\\Resources\\FBX\\" + stem + ".fbx");
	}

	uint8 ParseSpawnerTrigger(const std::string& triggerName)
	{
		if (triggerName == "Wave")
			return static_cast<uint8>(SpawnerTrigger::Wave);
		if (triggerName == "OnPlayerEnter")
			return static_cast<uint8>(SpawnerTrigger::OnPlayerEnter);
		return static_cast<uint8>(SpawnerTrigger::Timed);
	}

		uint8 ParseEnemyTypeName(const std::string& typeName)
		{
			if (typeName == "Pianoman")
				return static_cast<uint8>(EnemyType::Pianoman);
		if (typeName == "Bongoman")
			return static_cast<uint8>(EnemyType::Bongoman);
		if (typeName == "Obelisk")
			return static_cast<uint8>(EnemyType::Obelisk);
		if (typeName == "Fly")
			return static_cast<uint8>(EnemyType::Fly);
			if (typeName == "Brass")
				return static_cast<uint8>(EnemyType::Brass);
			return static_cast<uint8>(EnemyType::HornMan);
		}

		bool IsEnemyAllowedForScene(SceneId sceneId, uint8 enemyType)
		{
			if (enemyType == static_cast<uint8>(EnemyType::Obelisk))
				return sceneId == SceneId::SecondGame;
			if (enemyType == static_cast<uint8>(EnemyType::Brass))
				return sceneId == SceneId::ThirdGame;
			return true;
		}

		Entity SpawnFixedEnemy(World* world, EnemyType enemyType, const Vec3& position)
		{
			if (!world)
				return Entity{};

			InputCommand spawnCmd{};
			spawnCmd.SessionId = 0;
			spawnCmd.Type = PKT_Type::S2C_PKT_SPAWN;
			spawnCmd.StoreAs(EnemySpawnContext{ static_cast<uint8>(enemyType) });

			Entity spawned = PrefabFactory::Spawn(world, PrefabType::ENEMY, spawnCmd);
			if (!spawned.IsValid())
				return Entity{};

			if (TransformComponent* transform = world->GetComponent<TransformComponent>(spawned))
			{
				transform->mLocalPosition = position;
				transform->mWorldMatrix = Matrix::CreateTranslation(position);
			}

			if (GravityComponent* gravity = world->GetComponent<GravityComponent>(spawned))
			{
				gravity->mGround = position.y;
				gravity->mHight = position.y;
				gravity->mGravity = 0.0f;
				gravity->mFalling = false;
				gravity->mDropping = false;
				gravity->mGroundGraceLeft = 0.0f;
			}

			if (FlyComponent* fly = world->GetComponent<FlyComponent>(spawned))
				fly->mGround = position.y;

			return spawned;
		}

	std::vector<SpawnTableEntry> ParseSpawnTableEntries(const json& tableJson)
	{
		std::vector<SpawnTableEntry> entries;
		const auto& jsonEntries = RequireJson(tableJson, "entries");
		for (const auto& entryJson : jsonEntries)
		{
			SpawnTableEntry entry{};
			entry.enemyType = ParseEnemyTypeName(GetOptionalString(entryJson, "type", "HornMan"));
			entry.count = static_cast<int32>(GetOptionalFloat(entryJson, "count", 0.f));
			if (entry.count > 0)
				entries.push_back(entry);
		}
		return entries;
	}

	uint8 ParseInteractableKind(const std::string& name)
	{
		if (name == "HealPack")     return static_cast<uint8>(InteractableKind::HealPack);
		if (name == "ArmorPack")    return static_cast<uint8>(InteractableKind::ArmorPack);
		if (name == "JumpPad")      return static_cast<uint8>(InteractableKind::JumpPad);
		if (name == "SpeedPad")     return static_cast<uint8>(InteractableKind::SpeedPad);
		if (name == "DamageZone")   return static_cast<uint8>(InteractableKind::DamageZone);
		if (name == "ConquestZone") return static_cast<uint8>(InteractableKind::ConquestZone);
		if (name == "EscortZone")   return static_cast<uint8>(InteractableKind::EscortZone);
		if (name == "Checkpoint")   return static_cast<uint8>(InteractableKind::Checkpoint);
		return static_cast<uint8>(InteractableKind::None);
	}

	InteractableTarget ParseInteractableTarget(const std::string& name)
	{
		if (name == "All")   return InteractableTarget_All;
		if (name == "Enemy") return InteractableTarget_Enemy;
		if (name == "None")  return InteractableTarget_None;
		return InteractableTarget_Player; // 기본
	}

	void LoadInteractablesForScene(Scene* scene, World* world, const std::wstring& path)
	{
		if (!scene || !world)
			return;

		std::ifstream ifs(path);
		if (!ifs.is_open())
			throw std::runtime_error("Failed to open interactables json");

		json root;
		ifs >> root;

		const auto& items = RequireJson(root, "interactables");
		int loadedCount = 0;
		for (const auto& it : items)
		{
			const Vec3 position    = ParseVec3ArrayOrObject(RequireJson(it, "position"), 1.0f);
			const Vec3 halfExtents = ParseVec3ArrayOrObject(RequireJson(it, "halfExtents"), 1.0f);

			scene->SpawnInteractable(
				world,
				ParseInteractableKind(GetOptionalString(it, "kind", "None")),
				position,
				halfExtents,
				GetOptionalFloat(it, "valueA", 0.0f),
				GetOptionalFloat(it, "valueB", 0.0f),
				GetOptionalFloat(it, "cooldown", 0.0f),
				GetOptionalBool(it, "oneShot", false),
				ParseInteractableTarget(GetOptionalString(it, "targetMask", "Player")));
			++loadedCount;
		}
		std::cout << "[Gimmick] interactables loaded: " << loadedCount << std::endl;
	}


	// JSON 의 "playerSpawn" 을 읽어 스폰 지점 마커 엔티티를 배치
	bool LoadPlayerSpawnForScene(World* world, const std::wstring& path)
	{
		if (!world)
			return false;

		std::ifstream ifs(path);
		if (!ifs.is_open())
			return false;

		json root;
		ifs >> root;

		if (!root.contains("playerSpawn"))
			return false;

		Entity e = world->CreateEntity();
		PlayerSpawnComponent& spawn = world->AddComponent<PlayerSpawnComponent>(e);
		spawn.mPosition = ParseVec3ArrayOrObject(root["playerSpawn"], 1.0f);
		return true;
	}

	void LoadSpawnerTablesForScene(Scene* scene, World* world, const std::wstring& path)
	{
		if (!scene || !world)
			return;

		std::ifstream ifs(path);
		if (!ifs.is_open())
			throw std::runtime_error("Failed to open spawn table json");

		json root;
		ifs >> root;

		std::unordered_map<std::string, std::vector<SpawnTableEntry>> tableMap;
		const auto& jsonTables = RequireJson(root, "spawnTables");
		for (auto it = jsonTables.begin(); it != jsonTables.end(); ++it)
		{
			tableMap.emplace(it.key(), ParseSpawnTableEntries(it.value()));
		}

		const auto& jsonSpawners = RequireJson(root, "spawners");
		for (const auto& spawnerJson : jsonSpawners)
		{
			const Vec3 position = ParseVec3ArrayOrObject(RequireJson(spawnerJson, "position"), 1.0f);
			Entity spawner = scene->SpawnMonsterSpawner(
				world,
				position,
				GetOptionalFloat(spawnerJson, "interval", 5.0f),
				static_cast<int32>(GetOptionalFloat(spawnerJson, "maxAlive", 5.0f)),
				GetOptionalFloat(spawnerJson, "spawnRadius", 0.0f),
				ParseSpawnerTrigger(GetOptionalString(spawnerJson, "trigger", "Timed")),
				static_cast<uint8>(PrefabType::ENEMY),
				static_cast<int32>(GetOptionalFloat(spawnerJson, "maxTotal", -1.0f)),
				GetOptionalBool(spawnerJson, "startActive", true));

			SpawnerComponent* sp = world->GetComponent<SpawnerComponent>(spawner);
			if (!sp)
				continue;

			sp->mSpawnerId = GetOptionalString(spawnerJson, "id", "");
			sp->mSpawnTablePool.clear();
			sp->mCurrentSpawnPlan.clear();
			sp->mCurrentSpawnPlanIndex = 0;
			sp->mCurrentEntryRemaining = 0;
			sp->mSelectedTableIndex = -1;
			sp->mRandomSpawnFromTable = GetOptionalBool(spawnerJson, "randomSpawnFromTable", false);

				const auto& tablePoolJson = RequireJson(spawnerJson, "tablePool");
				for (const auto& tableNameJson : tablePoolJson)
				{
					const std::string tableName = tableNameJson.get<std::string>();
					auto found = tableMap.find(tableName);
					if (found != tableMap.end() && !found->second.empty())
					{
						std::vector<SpawnTableEntry> filteredEntries;
						filteredEntries.reserve(found->second.size());
						for (const SpawnTableEntry& entry : found->second)
						{
							if (IsEnemyAllowedForScene(scene->GetSceneId(), entry.enemyType))
								filteredEntries.push_back(entry);
						}
						if (!filteredEntries.empty())
							sp->mSpawnTablePool.push_back(std::move(filteredEntries));
					}
				}

			}

		// phase 별 스포너 세트
		std::unordered_map<std::string, std::vector<std::string>> phaseSets;
		if (root.contains("phaseSpawners"))
		{
			const auto& phaseJson = root["phaseSpawners"];
			for (auto it = phaseJson.begin(); it != phaseJson.end(); ++it)
			{
				if (!it.value().is_array())	// "_comment" 등 무시
					continue;

				std::vector<std::string> ids;
				for (const auto& idJson : it.value())
				{
					if (idJson.is_string())
						ids.push_back(idJson.get<std::string>());
				}
				phaseSets.emplace(it.key(), std::move(ids));
			}
		}
		scene->SetPhaseSpawnerSets(std::move(phaseSets));
	}
}



void Scene::Initialize()
{
	PrefabFactory::RegisterAllPrefabs();

	// Modified: Server terrain height now comes from NavMesh fallback and Jolt MeshShape raycast, so TerrainComponent is not spawned.

	//LoadCollisionJson(L"..\\Resources\\Json\\Map001_CRX.json");
	
	mGameMode = make_shared<WaveGameMode>();
	mWorld->Initialize();


}

void Scene::Update(float deltaTime)
{
	//mGameMode->PreUpdate(deltaTime);
	mWorld->Update(deltaTime);
	//mGameMode->PostUpdate(deltaTime);
}


void Scene::LoadJsonLevel(const wstring& path)
{

	int i = 0;
	try
	{
		LevelImportData level = RESOURCEMANAGER.LoadResourceJson(path);
		const std::wstring prefix = s2ws(level.levelName);

		for (const auto& inst : level.instances)
		{
			// 파일명만 추출
			std::string stem = filesystem::path(inst.fbx).filename().stem().string();
			std::wstring fbxPath = BuildLevelFbxPath(level.levelName, stem);
			shared_ptr<FBX> data = RESOURCEMANAGER.LoadFBXMeshes(fbxPath, prefix);

			if (!data)
			{
				std::cerr << "FBX load failed (null data): " << stem << "\n";
				break;
			}
			else if (data->GetColliders().empty()) {
				std::cerr << "FBX load failed Mesh (null data): " << stem << "\n";
				continue;
			}

			Entity entity = mWorld->CreateEntity();
			TransformComponent transform{};
			transform.mWorldMatrix = inst.worldMtx;

			TransformComponent& trans = mWorld->AddComponent<TransformComponent>(entity, transform);
			trans.mIsStatic = true;

			BoxColliderComponent& boxCollider = mWorld->AddComponent<BoxColliderComponent>(entity, 
				data->GetColliders().at(0)->GetOBB());
			// Jolt terrain raycast: keep the source render mesh so PhysicsWorld can build triangle terrain queries.
			boxCollider.mRayCastMesh = data->GetColliders().at(0);
			// Jolt terrain raycast: loaded level meshes must be static for PhysicsWorld rebuild and query registration.
			mWorld->AddComponent<StaticComponent>(entity);
		}
	}
	catch (const std::exception& e)
	{
		std::cerr << "Load failed: " << e.what() << "\n";
	}
}

void Scene::LoadCollisionJson(const wstring& path)
{
	{
		int loadedInstanceCount = 0;
		int loadedMeshCount = 0;
		int skippedCount = 0;
		try
		{
			LevelImportData level = RESOURCEMANAGER.LoadResourceJson(path);
			const std::wstring prefix = s2ws(level.levelName);
			auto physicsWorld = mWorld->GetPhysicsWorld();
			if (!physicsWorld)
				throw std::runtime_error("LoadCollisionJson requires World::Initialize before loading Jolt collision meshes");

			for (const auto& inst : level.instances)
			{
				if (inst.fbx.empty())
				{
					++skippedCount;
					continue;
				}

				const std::wstring fbxPath = ResolveCollisionFbxPath(inst.fbx);
				if (fbxPath.empty())
				{
					++skippedCount;
					continue;
				}

				shared_ptr<FBX> collisionFbx = RESOURCEMANAGER.LoadFBXMeshes(fbxPath, prefix);
				if (!collisionFbx)
				{
					++skippedCount;
					continue;
				}

				const vector<shared_ptr<CollisionMesh>> colliders = collisionFbx->GetColliders();
				if (colliders.empty())
				{
					++skippedCount;
					std::cerr << "LoadCollisionJson skipped collision mesh without converted .mesh data: " << ws2s(fbxPath) << "\n";
					continue;
				}

				Entity entity = mWorld->CreateEntity();

				TransformComponent transform{};
				transform.mWorldMatrix = inst.worldMtx;
				TransformComponent& trans = mWorld->AddComponent<TransformComponent>(entity, transform);
				trans.mIsStatic = true;
				mWorld->AddComponent<StaticComponent>(entity);

				bool registeredAnyMesh = false;
				for (const shared_ptr<CollisionMesh>& colliderMesh : colliders)
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

			std::cout << "Loaded Jolt collision instances: " << loadedInstanceCount
				<< ", meshes: " << loadedMeshCount
				<< ", skipped: " << skippedCount << std::endl;
		}
		catch (const std::exception& e)
		{
			std::cerr << "Load failed: " << e.what() << "\n";
		}
	}
	return;
}

#if 0
// Legacy primitive collision loader disabled after switching static map collision to Jolt MeshShape.

	int loadedCount = 0;
	int loadedSphereCount = 0;
	try
	{
		LevelImportData level = RESOURCEMANAGER.LoadResourceJson(path);

		shared_ptr<Mesh> cubeMesh = RESOURCEMANAGER.LoadMCubeMesh();
		if (!cubeMesh)
			throw std::runtime_error("LoadCollisionJson failed to load MCube mesh");

		std::vector<Vec3> localVertices;
		for (const Vertex& vertex : cubeMesh->GetVertexBuffer())
			localVertices.push_back(vertex.pos);

		if (localVertices.empty())
			throw std::runtime_error("LoadCollisionJson MCube mesh has no vertices");

		BoundingOrientedBox localObb;
		BoundingOrientedBox::CreateFromPoints(localObb, localVertices.size(), localVertices.data(), sizeof(Vec3));
		// 수정 내용
		// Map001_CRX.json 의 CRX_Cube 는 원본 메시 크기가 100 x 100 x 100 이다.
		// LoadMCubeMesh 는 1 x 1 x 1 디버그 큐브라서 OBB 반경을 100 배 보정해 실제 반경 50 을 맞춘다.
		constexpr float kCrxCubeSourceSize = 100.0f;
		localObb.Extents.x *= kCrxCubeSourceSize;
		localObb.Extents.y *= kCrxCubeSourceSize;
		localObb.Extents.z *= kCrxCubeSourceSize;

		for (const auto& inst : level.instances)
		{
			const bool isCollisionCube =
				inst.fbx.find("CRX_Cube") != std::string::npos ||
				inst.staticMeshAsset.find("CRX_Cube") != std::string::npos ||
				inst.componentName.find("CRX_Cube") != std::string::npos;
			const bool isCollisionSphere =
				inst.fbx.find("CRX_Sphere") != std::string::npos ||
				inst.staticMeshAsset.find("CRX_Sphere") != std::string::npos ||
				inst.componentName.find("CRX_Sphere") != std::string::npos;
			if (!isCollisionCube && !isCollisionSphere)
				continue;

			Entity entity = mWorld->CreateEntity();

			// 수정 내용
			// Map001_CRX.json 의 dx transform 을 서버 정적 충돌체 transform 으로 저장한다.
			// TransformSystem 은 static transform 을 갱신하지 않으므로 PhysicsWorld 가 이 worldMatrix 로 OBB 를 만든다.
			TransformComponent transform{};
			transform.mWorldMatrix = inst.worldMtx;
			TransformComponent& trans = mWorld->AddComponent<TransformComponent>(entity, transform);
			trans.mIsStatic = true;

			if (isCollisionCube)
			{
				// 수정 내용
				// CRX_Cube 는 원본 100 단위 박스이고 JSON scale basis location 이 배치 크기와 방향을 가진다.
				// 따라서 보정된 localObb 를 보관하고 PhysicsWorld::UpdateWorldOBB 에서 worldMatrix 로 변환한다.
				BoxColliderComponent& boxCollider = mWorld->AddComponent<BoxColliderComponent>(entity, localObb, transform.mWorldMatrix);
				(void)boxCollider;
				++loadedCount;
			}
			else
			{
				// 수정 내용
				// CRX_Sphere 는 지름 100 인 원본 구이므로 로컬 반지름 50 을 가진 SphereColliderComponent 로 등록한다.
				// PhysicsWorld::UpdateWorldSphere 에서 JSON worldMatrix scale 을 반지름에 적용한다.
				SphereColliderComponent& sphereCollider = mWorld->AddComponent<SphereColliderComponent>(entity, 50.0f);
				(void)sphereCollider;
				++loadedSphereCount;
			}

			mWorld->AddComponent<StaticComponent>(entity);
		}

		if ((loadedCount + loadedSphereCount) > 0 && mWorld->GetPhysicsWorld())
		{
			// 수정 내용
			// 충돌 JSON 을 World::Initialize 이후에 다시 로드해도 정적 BVH 가 새 충돌체를 즉시 포함하도록 갱신한다.
			mWorld->GetPhysicsWorld()->SyncStaticBVHIfNeeded();
		}

		std::cout << "Loaded collision boxes: " << loadedCount << ", spheres: " << loadedSphereCount << std::endl;
	}
	catch (const std::exception& e)
	{
		std::cerr << "Load failed: " << e.what() << "\n";
	}
}

// halfExtents = 트리거 영역 반경 / valueA,valueB = kind별 파라미터 / cooldown = 0이면 지속형.
#endif


void Scene::LoadNavMesh(const wstring& path)
{
	mNavMeshPath = path;
	shared_ptr<NavMesh> navMesh = RESOURCEMANAGER.Get<NavMesh>(mNavMeshPath);
	if (navMesh == nullptr)
	{
		navMesh = make_shared<NavMesh>();
		navMesh->Load(ws2s(mNavMeshPath));
		RESOURCEMANAGER.Add<NavMesh>(mNavMeshPath, navMesh);
	}
	if (navMesh && navMesh->mDtNavMesh)
		mWorld->GetNavSystem()->Initialize(navMesh);
}

Entity Scene::SpawnInteractable(World* world,
	uint8 kind,
	const Vec3& position,
	const Vec3& halfExtents,
	float valueA,
	float valueB,
	float cooldown,
	bool  oneShot,
	InteractableTarget targetMask,
	SkillType buffType)
{
	Entity e = world->CreateEntity();

	TransformComponent transform{};
	transform.mLocalPosition = position;
	transform.mWorldPosition = position;
	transform.mWorldMatrix = Matrix::CreateTranslation(position);
	transform.mIsStatic = true;
	world->AddComponent<TransformComponent>(e, transform);

	// 트리거 콜라이더 (물리 해결 없음)
	BoxColliderComponent& col = world->AddComponent<BoxColliderComponent>(e, halfExtents);
	col.bIsTrigger = true;

	// Interactable
	InteractableComponent& inter = world->AddComponent<InteractableComponent>(e);
	inter.mKind = static_cast<InteractableKind>(kind);
	inter.mValueA = valueA;
	inter.mValueB = valueB;
	inter.mCooldown = cooldown;
	inter.mOneShot = oneShot;
	inter.mBuffType = buffType;
	inter.mTargetMask = InteractableTarget_Player;
	
	world->AddComponent<NetEntityComponent>(e, world, e);

	return e;
}

// 주기 스폰 포인트.
// trigger가 Timed가 아니면 startActive를 false로 두고 외부(이벤트/게임모드)가 깨우도록 한다.
Entity Scene::SpawnMonsterSpawner(World* world,
	const Vec3& position,
	float interval,
	int32 maxAlive,
	float spawnRadius,
	uint8 triggerRaw,
	uint8 prefabTypeRaw,
	int32 maxTotal,
	bool  startActive)
{
	Entity e = world->CreateEntity();

	TransformComponent transform{};
	transform.mLocalPosition = position;
	transform.mWorldPosition = position;
	transform.mWorldMatrix = Matrix::CreateTranslation(position);
	transform.mIsStatic = true;
	world->AddComponent<TransformComponent>(e, transform);

	SpawnerComponent& sp = world->AddComponent<SpawnerComponent>(e);
	sp.mTrigger = static_cast<SpawnerTrigger>(triggerRaw);
	sp.mPrefabType = static_cast<PrefabType>(prefabTypeRaw);
	sp.mInterval = interval;
	sp.mMaxAlive = maxAlive;
	sp.mSpawnRadius = spawnRadius;
	sp.mMaxTotal = maxTotal;
	sp.mActive = startActive;

	world->AddComponent<NetEntityComponent>(e, world, e);
	sp.mNextSpawnTime = 0.0f; // 다음 Update에서 즉시 후보
	return e;
}

void Scene::SetPhaseSpawnerSets(std::unordered_map<std::string, std::vector<std::string>> sets)
{
	mPhaseSpawnerSets = std::move(sets);

	mPhaseManagedSpawnerIds.clear();
	for (const auto& [phaseKey, ids] : mPhaseSpawnerSets)
		for (const std::string& id : ids)
			mPhaseManagedSpawnerIds.insert(id);

	if (mPhaseManagedSpawnerIds.empty() || !mWorld->HasComponentPool<SpawnerComponent>())
		return;

	// phase 관리 대상은 startActive flase
	for (Entity e : mWorld->GetEntitiesWithComponent<SpawnerComponent>())
	{
		SpawnerComponent* sp = mWorld->GetComponent<SpawnerComponent>(e);
		if (sp && mPhaseManagedSpawnerIds.count(sp->mSpawnerId) > 0)
			sp->mActive = false;
	}
}

void Scene::ApplyPhaseSpawnerSet(const std::string& phaseKey)
{
	// phaseSpawners 를 정의하지 않은 맵은 기존 동작(Timed/startActive) 그대로 둔다.
	if (mPhaseManagedSpawnerIds.empty())
		return;
	if (!mWorld->HasComponentPool<SpawnerComponent>())
		return;

	static const std::vector<std::string> kEmpty;
	const auto found = mPhaseSpawnerSets.find(phaseKey);
	const std::vector<std::string>& activeIds = (found != mPhaseSpawnerSets.end()) ? found->second : kEmpty;

	const float now = GetServerTotalTimeSeconds();
	auto em = mWorld->GetEventManager();
	int activated = 0;
	for (Entity e : mWorld->GetEntitiesWithComponent<SpawnerComponent>())
	{
		SpawnerComponent* sp = mWorld->GetComponent<SpawnerComponent>(e);
		if (!sp || sp->mSpawnerId.empty())
			continue;
		if (mPhaseManagedSpawnerIds.count(sp->mSpawnerId) == 0) // phase 관리 대상이 아닌 스포너
			continue;

		const bool shouldBeActive =
			std::find(activeIds.begin(), activeIds.end(), sp->mSpawnerId) != activeIds.end();

		const bool wasActive = sp->mActive;

		if (shouldBeActive)
		{
			// phase 마다 reset
			sp->mActive = true;
			sp->mTotalSpawned = 0;

			sp->mTablePoolCycleStarted = false;
			sp->mCurrentTableCompletedAfterSpawn = false;
			sp->mRemainingTableIndices.clear();
			sp->mSelectedTableIndex = -1;
			sp->mCurrentSpawnPlan.clear();
			sp->mCurrentSpawnPlanIndex = 0;
			sp->mCurrentEntryRemaining = 0;
			sp->mNextSpawnTime = now;	// 즉시 첫 스폰 허용
			++activated;
		}
		else
		{
			sp->mActive = false;	// 이미 스폰된 개체는 그대로 둔다
		}

		// 활성 상태가 실제로 바뀐 스포너만 클라 마커 VFX 토글을 broadcast
		if (em && sp->mActive != wasActive)
			em->Enqueue<EvInteractableStateChanged>(EvInteractableStateChanged{ e, sp->mActive });
	}

	std::cout << "[PhaseSpawner] phase=" << phaseKey << " active=" << activated
		<< "/" << mPhaseManagedSpawnerIds.size() << std::endl;
}

void Scene::SetGameMode(shared_ptr<GameMode>& gameMode)
{
	if (gameMode) {
		mGameMode = gameMode;

		mGameMode->SetScene(shared_from_this()); // GameMode에 씬 참조 전달
		
	}
}

void FirstScene::Initialize()
{
	//PlayerPrefab p{ mWorld.get()};
	PrefabFactory::RegisterAllPrefabs();

	// Modified: Server terrain height now comes from NavMesh fallback and Jolt MeshShape raycast, so TerrainComponent is not spawned.

	const std::wstring gimmickPath = L"../Resources/Json/Map001_Gimmicks.json";

	// 트럭 스폰 (위치는 JSON truckSpawn 에서)
	Vec3 truckSpawn(-4902.f, 135.f, -9240.f);
	ReadOptionalVec3(gimmickPath, "truckSpawn", truckSpawn);

	InputCommand dummy{};
	Entity truck = PrefabFactory::Spawn(mWorld.get(), PrefabType::TRUCK, dummy);
	

	PathLoadComponent* plc = mWorld->GetComponent<PathLoadComponent>(truck);
	plc->mBaseOffset = truckSpawn;

	TransformComponent* trans = mWorld->GetComponent< TransformComponent>(truck);
	trans->mLocalPosition = truckSpawn;

	
	auto waveMode = make_shared<WaveGameMode>();
	waveMode->SetCompletionScene(SceneId::SecondGame); // 마지막 Conquest 클리어 후 SecondGame 으로 전환
	shared_ptr<GameMode> gameMode = waveMode;
	SetGameMode(gameMode);


	mWorld->Initialize();
	LoadCollisionJson(L"..\\Resources\\Json\\Map001_Nav_Export.json");
	LoadNavMesh(L"NavMesh_FirstGame");

	mWorld->GetSystemManager()->RegisterSystem<NetRecvSystem>();       // 1. 입력 수신
	mWorld->GetSystemManager()->RegisterSystem<GamePreRuleSystem>(mGameMode);     // 1-1. 게임 룰 체크 (예: 승패 조건, 라운드 진행 등)
	mWorld->GetSystemManager()->RegisterSystem<PlayerSystem>();
	mWorld->GetSystemManager()->RegisterSystem<BuffSystem>();          // 5. 버프 틱/만료 처리// 2. 플레이어 입력 이동 상태 반영
	mWorld->GetSystemManager()->RegisterSystem<EnemySystem>();         // 3. 적 AI → 이동 상태 반영
	mWorld->GetSystemManager()->RegisterSystem<BeatSystem>();          // 4. 비트 타이밍
	mWorld->GetSystemManager()->RegisterSystem<PlayerInputSystem>();   // 5. 입력 처리
	mWorld->GetSystemManager()->RegisterSystem<MovementSystem>();      // 6. mLocalPosition += v*dt
	
	mWorld->GetSystemManager()->RegisterSystem<PathFollowSystem>();    // 6-1. PayloadPathData 추종 (화물·시네마틱 카메라)
	mWorld->GetSystemManager()->RegisterSystem<TransformSystem>();     // 7. mWorldMatrix = f(mLocalPosition)  이동 후 재계산
	//mWorld->GetSystemManager()->RegisterSystem<CameraSystem>(); //  8. 카메라 업데이트
	mWorld->GetSystemManager()->RegisterSystem<MeleeAttackSystem>();   // 9. 근접 공격
	mWorld->GetSystemManager()->RegisterSystem<BulletFireEventSystem>(); // 10. 투사체 발사
	mWorld->GetSystemManager()->RegisterSystem<CollisionSystem>();     // 11. 최신 mWorldMatrix로 충돌 판정
	mWorld->GetSystemManager()->RegisterSystem<InteractionSystem>();   // 11-1. 힐팩/점프대 등 맵 상호작용 트리거 검사
	mWorld->GetSystemManager()->RegisterSystem<SpawnerSystem>();       // 11-2. 주기/이벤트 기반 몬스터 스폰
	mWorld->GetSystemManager()->RegisterSystem<DamageSystem>();        // 12. 데미지/회복 처리
	mWorld->GetSystemManager()->RegisterSystem<DeathSystem>();			// 사망/리스폰 처리
	mWorld->GetSystemManager()->RegisterSystem<PlayerNavValidationSystem>(); // 13. Jolt 위치를 NavMesh 표면에 투영(적 AI 길찾기 목표용), Transform 비파괴
	mWorld->GetSystemManager()->RegisterSystem<GamePostRuleSystem>(mGameMode);      // 13-1. 게임 룰 적용 (예: 점령지 점유 상태 업데이트)
	mWorld->GetSystemManager()->RegisterSystem<GameNetRuleSystem>(mGameMode);      // 13-1. 게임 룰 적용 (예: 점령지 점유 상태 업데이트)
	mWorld->GetSystemManager()->RegisterSystem<NetSendSystem>();       // 14. 상태 송신 (가장 마지막)

	


	// 플레이어 스폰/힐팩/점프대/점령지
	LoadPlayerSpawnForScene(mWorld.get(), gimmickPath);
	LoadInteractablesForScene(this, mWorld.get(), gimmickPath);


	//// 몬스터 주기 스폰 포인트. 5초마다 스폰, 동시 최대 3마리, 반경 200 안에 랜덤 배치.
	//SpawnMonsterSpawner(mWorld.get(),
	//	Vec3(0.0f, 0.0f, 0.0f),
	//	/*interval=*/5.0f,
	//	/*maxAlive=*/3,
	//	/*spawnRadius=*/200.0f,
	//	/*triggerRaw=*/static_cast<uint8>(SpawnerTrigger::Timed),
	//	/*prefabTypeRaw=*/static_cast<uint8>(PrefabType::ENEMY),
	//	/*maxTotal=*/-1,
	//	/*startActive=*/true);

	//// 하이브리드: 플레이어가 볼륨에 진입하면 그 자리부터 3마리가 1초 간격 웨이브로 튀어나옴.
	//// 동일 Entity에 Interactable(OneShot) + Spawner(OnPlayerEnter)를 부착해 매핑 없이 깨운다.
	//{
	//	const Vec3 triggerPos(300.0f, 0.0f, 300.0f);
	//	Entity e = SpawnInteractable(mWorld.get(),
	//		static_cast<uint8>(InteractableKind::None), // 소비만 감지, 실제 효과는 kind에 의존하지 않음
	//		triggerPos,
	//		Vec3(100.0f, 50.0f, 100.0f),
	//		0.0f, 0.0f,
	//		/*cooldown=*/0.0f,
	//		/*oneShot=*/true);

	//	// 동일 엔티티에 스포너도 장착 — OnPlayerEnter로 켜질 때까지 대기
	//	SpawnerComponent& sp = mWorld->AddComponent<SpawnerComponent>(e);
	//	sp.mTrigger = SpawnerTrigger::OnPlayerEnter;
	//	sp.mPrefabType = PrefabType::ENEMY;
	//	sp.mInterval = 1.0f;
	//	sp.mMaxAlive = 3;
	//	sp.mMaxTotal = 3;
	//	sp.mSpawnRadius = 150.0f;
	//	sp.mActive = false; // EvInteractableConsumed로 true가 된다
	//}

	LoadSpawnerTablesForScene(this, mWorld.get(), gimmickPath);

	gameMode->Initialize();

	mSceneId = SceneId::FirstGame;



	
}

void SecondScene::Initialize()
{
	PrefabFactory::RegisterAllPrefabs();

	// Conquest 전용 게임 모드
	auto waveMode = make_shared<WaveGameMode>();
	waveMode->SetCompletionScene(SceneId::ThirdGame); // 모든 Conquest 클리어 후 보스전(ThirdGame)으로 전환
	{
		std::deque<WaveGameMode::PhaseFactory> phases;
		phases.push_back([] { return new PreparePhase(); });
		phases.push_back([] { return new ConquestPhase(/*zoneId=*/1, /*requiredSeconds=*/30.f); });
		phases.push_back([] { return new ConquestPhase(/*zoneId=*/2, /*requiredSeconds=*/30.f); });
		phases.push_back([] { return new ClearPhase(3.0f); });
		waveMode->SetInitialPhases(std::move(phases));
	}
	shared_ptr<GameMode> gameMode = waveMode;
	SetGameMode(gameMode);

	mWorld->Initialize();
	LoadCollisionJson(L"..\\Resources\\Json\\MapDesert_Nav_Export.json");
	LoadNavMesh(L"NavMesh_SecondGame");

	mWorld->GetSystemManager()->RegisterSystem<NetRecvSystem>();       // 1. 입력 수신
	mWorld->GetSystemManager()->RegisterSystem<GamePreRuleSystem>(mGameMode);     // 1-1. 게임 룰 PreUpdate
	mWorld->GetSystemManager()->RegisterSystem<PlayerSystem>();        // 2. 플레이어 입력 → 이동 상태 반영
	mWorld->GetSystemManager()->RegisterSystem<BuffSystem>();          // 2-1. 버프 틱/만료 처리
	mWorld->GetSystemManager()->RegisterSystem<EnemySystem>();         // 3. 적 AI → 이동 상태 반영
	mWorld->GetSystemManager()->RegisterSystem<BeatSystem>();          // 4. 비트 타이밍
	mWorld->GetSystemManager()->RegisterSystem<PlayerInputSystem>();   // 5. 입력 처리
	mWorld->GetSystemManager()->RegisterSystem<MovementSystem>();      // 6. mLocalPosition += v*dt
	mWorld->GetSystemManager()->RegisterSystem<PathFollowSystem>();    // 6-1. PayloadPathData 추종
	mWorld->GetSystemManager()->RegisterSystem<TransformSystem>();     // 7. mWorldMatrix = f(mLocalPosition) ← 이동 후 재계산
	mWorld->GetSystemManager()->RegisterSystem<MeleeAttackSystem>();   // 9. 근접 공격
	mWorld->GetSystemManager()->RegisterSystem<BulletFireEventSystem>(); // 10. 투사체 발사
	mWorld->GetSystemManager()->RegisterSystem<CollisionSystem>();     // 11. 최신 mWorldMatrix로 충돌 판정
	mWorld->GetSystemManager()->RegisterSystem<InteractionSystem>();   // 11-1. 힐팩/점프대 등 맵 상호작용 트리거 검사
	mWorld->GetSystemManager()->RegisterSystem<SpawnerSystem>();       // 11-2. 주기/이벤트 기반 몬스터 스폰
	mWorld->GetSystemManager()->RegisterSystem<DamageSystem>();        // 12. 데미지/회복 처리
	mWorld->GetSystemManager()->RegisterSystem<DeathSystem>();         // 사망/리스폰 처리
	mWorld->GetSystemManager()->RegisterSystem<PlayerNavValidationSystem>(); // 13. Jolt 위치를 NavMesh 표면에 투영
	mWorld->GetSystemManager()->RegisterSystem<GamePostRuleSystem>(mGameMode);  // 13-1. 게임 룰 PostUpdate (점령 판정)
	mWorld->GetSystemManager()->RegisterSystem<GameNetRuleSystem>(mGameMode);   // 13-2. 게임 룰 상태 방송
	mWorld->GetSystemManager()->RegisterSystem<NetSendSystem>();       // 14. 상태 송신 (가장 마지막)

	// 기믹(플레이어 스폰/점령지/힐팩/점프대/스포너) 
	//  첫 ConquestPhase::Enter 가 점령지를 알아서 매핑
		const std::wstring gimmickPath = L"../Resources/Json/MapDesert_Gimmicks.json";
		LoadPlayerSpawnForScene(mWorld.get(), gimmickPath);          // 플레이어 스폰 위치
		LoadInteractablesForScene(this, mWorld.get(), gimmickPath);  // 점령지/힐팩/점프대 등
		LoadSpawnerTablesForScene(this, mWorld.get(), gimmickPath);  // 몬스터 스포너
		SpawnFixedEnemy(mWorld.get(), EnemyType::Obelisk, Vec3(-3389.0f,104.0f, -5221.0f));


	gameMode->Initialize();

	mSceneId = SceneId::SecondGame;
}

void ThirdScene::Initialize()
{
	PrefabFactory::RegisterAllPrefabs();


	auto waveMode = make_shared<WaveGameMode>();
	waveMode->SetCompletionScene(SceneId::VGame); // 보스전 클리어 후 승리 화면으로 전환
	{
		std::deque<WaveGameMode::PhaseFactory> phases;
		phases.push_back([] { return new PreparePhase(); });
		phases.push_back([] { return new BossPhase(/*zoneId=*/0); });
		phases.push_back([] { return new ClearPhase(3.0f); });
		waveMode->SetInitialPhases(std::move(phases));
	}
	shared_ptr<GameMode> gameMode = waveMode;
	SetGameMode(gameMode);

	mWorld->Initialize();

	// 임시
	LoadCollisionJson(L"..\\Resources\\Json\\Map003_Nav_Export.json");
	LoadNavMesh(L"NavMesh_ThirdGame");

	mWorld->GetSystemManager()->RegisterSystem<NetRecvSystem>();       // 1. 입력 수신
	mWorld->GetSystemManager()->RegisterSystem<GamePreRuleSystem>(mGameMode);     // 1-1. 게임 룰 PreUpdate
	mWorld->GetSystemManager()->RegisterSystem<PlayerSystem>();        // 2. 플레이어 입력 → 이동 상태 반영
	mWorld->GetSystemManager()->RegisterSystem<BuffSystem>();          // 2-1. 버프 틱/만료 처리
	mWorld->GetSystemManager()->RegisterSystem<EnemySystem>();         // 3. 적 AI → 이동 상태 반영
	mWorld->GetSystemManager()->RegisterSystem<BeatSystem>();          // 4. 비트 타이밍
	mWorld->GetSystemManager()->RegisterSystem<PlayerInputSystem>();   // 5. 입력 처리
	mWorld->GetSystemManager()->RegisterSystem<MovementSystem>();      // 6. mLocalPosition += v*dt
	mWorld->GetSystemManager()->RegisterSystem<PathFollowSystem>();    // 6-1. PayloadPathData 추종
	mWorld->GetSystemManager()->RegisterSystem<TransformSystem>();     // 7. mWorldMatrix = f(mLocalPosition)
	mWorld->GetSystemManager()->RegisterSystem<MeleeAttackSystem>();   // 9. 근접 공격
	mWorld->GetSystemManager()->RegisterSystem<BulletFireEventSystem>(); // 10. 투사체 발사
	mWorld->GetSystemManager()->RegisterSystem<CollisionSystem>();     // 11. 충돌 판정
	mWorld->GetSystemManager()->RegisterSystem<InteractionSystem>();   // 11-1. 맵 상호작용
	mWorld->GetSystemManager()->RegisterSystem<SpawnerSystem>();       // 11-2. 몬스터 스폰
	mWorld->GetSystemManager()->RegisterSystem<DamageSystem>();        // 12. 데미지/회복
	mWorld->GetSystemManager()->RegisterSystem<DeathSystem>();         // 사망/리스폰
	mWorld->GetSystemManager()->RegisterSystem<PlayerNavValidationSystem>(); // 13. NavMesh 투영
	mWorld->GetSystemManager()->RegisterSystem<GamePostRuleSystem>(mGameMode);  // 13-1. 게임 룰 PostUpdate
	mWorld->GetSystemManager()->RegisterSystem<GameNetRuleSystem>(mGameMode);   // 13-2. 게임 룰 상태 방송
	mWorld->GetSystemManager()->RegisterSystem<NetSendSystem>();       // 14. 상태 송신 (가장 마지막)

		// 임시
		const std::wstring gimmickPath = L"../Resources/Json/Map003_Gimmicks.json";
		LoadPlayerSpawnForScene(mWorld.get(), gimmickPath);
		SpawnFixedEnemy(mWorld.get(), EnemyType::Brass, Vec3(-1600.0f, 0.0f, 5.0f));
		SpawnFixedEnemy(mWorld.get(), EnemyType::Obelisk, Vec3(-1600.0f, 0.0f, 500.0f));
		SpawnFixedEnemy(mWorld.get(), EnemyType::Obelisk, Vec3(-1600.0f, 0.0f, -470.0f));

		gameMode->Initialize();

	mSceneId = SceneId::ThirdGame;
}

void LobbyScene::Initialize()
{
	mWorld->Initialize();
	mSceneId = SceneId::Lobby;

	shared_ptr<GameMode> gameMode = make_shared<LobbyGameMode>();
	SetGameMode(gameMode);
}

void VictoryScene::Initialize()
{
	mWorld->Initialize();
	mSceneId = SceneId::VGame;

	shared_ptr<GameMode> gameMode = make_shared<ResultGameMode>();
	SetGameMode(gameMode);
}

void LoseScene::Initialize()
{
	mWorld->Initialize();
	mWorld->GetSystemManager()->RegisterSystem<NetRecvSystem>();       // 1. 입력 수신

	mWorld->GetSystemManager()->RegisterSystem<NetSendSystem>();       // 14. 상태 송신 (가장 마지막)
	mSceneId = SceneId::LGame;

	shared_ptr<GameMode> gameMode = make_shared<ResultGameMode>();
	SetGameMode(gameMode);
}
