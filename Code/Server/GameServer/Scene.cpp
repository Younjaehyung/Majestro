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
#include "PathLoadComponent.h"
#include "LevelImport.h"
#include "Mesh.h"
#include "Prefab.h"

#include "GameMode.h"
#include "SystemManager.h"
#include "CameraSystem.h"
#include "TransformSystem.h"
#include "PlayerSystem.h"
#include "EnemySystem.h"
#include "BeatSystem.h"
#include "MovementSystem.h"
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
#include "SpawnerSystem.h"
#include "SpawnerComponent.h"
#include "GameRuleSystem.h"
#include "PathFollowSystem.h"

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
}



void Scene::Initialize()
{
	PrefabFactory::RegisterAllPrefabs();

	TerrainPrefab terrain{ mWorld.get()};

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

		for (const auto& inst : level.instances)
		{
			// 파일명만 추출
			std::string name = filesystem::path(inst.fbx).filename().stem().string();
			name = "..\\Resources\\FBX\\" + name + ".fbx";
			shared_ptr<FBX> data = RESOURCEMANAGER.LoadFBXMeshes(s2ws(name));

			if (!data)
			{
				std::cerr << "FBX load failed (null data): " << name << "\n";
				break;
			}
			else if (data->GetColliders().empty()) {
				std::cerr << "FBX load failed Mesh (null data): " << name << "\n";
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

				shared_ptr<FBX> collisionFbx = RESOURCEMANAGER.LoadFBXMeshes(fbxPath);
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

				// Modified: Store the JSON TRS on the server entity and register collision FBX triangles in Jolt.
				// The Jolt body bakes this matrix into MeshShape vertices so server collision matches client debug placement.
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

	TerrainPrefab terrain{ mWorld.get() };
	
	// 트럭 스폰
	InputCommand dummy{};
	Entity truck = PrefabFactory::Spawn(mWorld.get(), PrefabType::TRUCK, dummy);
	PathLoadComponent* plc = mWorld->GetComponent<PathLoadComponent>(truck);
	TransformComponent* trans = mWorld->GetComponent< TransformComponent>(truck);
	plc->mBaseOffset = Vec3(-4902.f, 135.f, -9240.f);
	trans -> mLocalPosition = Vec3(-4902.f, 135.f, -9240.f);

	shared_ptr<GameMode> gameMode = make_shared<WaveGameMode>();
	SetGameMode(gameMode);
	gameMode->Initialize();

	mWorld->Initialize();
	LoadCollisionJson(L"..\\Resources\\Json\\Map001_Nav_Export.json");
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
	mWorld->GetSystemManager()->RegisterSystem<CameraSystem>();        // 8. 카메라 업데이트
	mWorld->GetSystemManager()->RegisterSystem<MeleeAttackSystem>();   // 9. 근접 공격
	mWorld->GetSystemManager()->RegisterSystem<BulletFireEventSystem>(); // 10. 투사체 발사
	mWorld->GetSystemManager()->RegisterSystem<CollisionSystem>();     // 11. 최신 mWorldMatrix로 충돌 판정
	mWorld->GetSystemManager()->RegisterSystem<InteractionSystem>();   // 11-1. 힐팩/점프대 등 맵 상호작용 트리거 검사
	mWorld->GetSystemManager()->RegisterSystem<SpawnerSystem>();       // 11-2. 주기/이벤트 기반 몬스터 스폰
	mWorld->GetSystemManager()->RegisterSystem<DamageSystem>();        // 12. 데미지/회복 처리
	mWorld->GetSystemManager()->RegisterSystem<PlayerNavValidationSystem>(); // 13. Nav 검증
	mWorld->GetSystemManager()->RegisterSystem<GamePostRuleSystem>(mGameMode);      // 13-1. 게임 룰 적용 (예: 점령지 점유 상태 업데이트)
	mWorld->GetSystemManager()->RegisterSystem<GameNetRuleSystem>(mGameMode);      // 13-1. 게임 룰 적용 (예: 점령지 점유 상태 업데이트)
	mWorld->GetSystemManager()->RegisterSystem<NetSendSystem>();       // 14. 상태 송신 (가장 마지막)

	


	// 힐팩: 10초 쿨다운, 75 회복. 월드 원점 근처에 배치.
	SpawnInteractable(mWorld.get(),
		static_cast<uint8>(InteractableKind::HealPack),
		Vec3(-5843.0f, 278.0f, -3523.0f),
		Vec3(  3.0f, 3.0f,   3.0f),
		/*valueA(회복량)=*/75.0f,
		/*valueB=*/0.0f,
		/*cooldown=*/10.0f,
		/*oneShot=*/false);

	// 점프대: 지속 활성, 위로 1200 / 전방 400 임펄스.
	SpawnInteractable(mWorld.get(),
		static_cast<uint8>(InteractableKind::JumpPad),
		Vec3(-2337.0f, 142.0f, -4987.0f),
		Vec3(  150.0f, 5.0f,   150.0f),
		/*valueA(Y 임펄스)=*/1200.0f,
		/*valueB(전방 임펄스)=*/5.0f,
		/*cooldown=*/0.5f,
		/*oneShot=*/false);


	// 점령지 1
	SpawnInteractable(mWorld.get(),
		static_cast<uint8>(InteractableKind::ConquestZone),
		Vec3(-500.0f, -127.0f, 1240.0f),
		Vec3(  300.0f, 300.0f,   300.0f),
		/*valueA(점령지 넘버)=*/1.0f,
		/*valueB(미정)=*/0.0f,
		/*cooldown=*/0.0f,
		/*oneShot=*/false,
		InteractableTarget_All
	);


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


	SpawnMonsterSpawner(mWorld.get(),
		Vec3(-4910.0f, 142.0f, -1623.0f),
		/*interval=*/8.0f,
		/*maxAlive=*/2,
		/*spawnRadius=*/150.0f,
		/*triggerRaw=*/static_cast<uint8>(SpawnerTrigger::Timed),
		/*prefabTypeRaw=*/static_cast<uint8>(PrefabType::ENEMY),
		/*maxTotal=*/6,
		/*startActive=*/true);

	SpawnMonsterSpawner(mWorld.get(),
		Vec3(-2307.0f, 740.0f, -4097.0f),
		/*interval=*/8.0f,
		/*maxAlive=*/2,
		/*spawnRadius=*/150.0f,
		/*triggerRaw=*/static_cast<uint8>(SpawnerTrigger::Timed),
		/*prefabTypeRaw=*/static_cast<uint8>(PrefabType::ENEMY),
		/*maxTotal=*/6,
		/*startActive=*/true);

	SpawnMonsterSpawner(mWorld.get(),
		Vec3(-5943.0f, 142.0f, -5637.0f),
		/*interval=*/8.0f,
		/*maxAlive=*/2,
		/*spawnRadius=*/150.0f,
		/*triggerRaw=*/static_cast<uint8>(SpawnerTrigger::Timed),
		/*prefabTypeRaw=*/static_cast<uint8>(PrefabType::ENEMY),
		/*maxTotal=*/6,
		/*startActive=*/true);

	mSceneId = SceneId::FirstGame;



	
}

void SecondScene::Initialize()
{
	//PlayerPrefab p{ mWorld.get()};
	PrefabFactory::RegisterAllPrefabs();

	TerrainPrefab terrain{ mWorld.get() };

	//LoadCollisionJson(L"..\\Resources\\Json\\Map001_CRX.json");
	mWorld->Initialize();
	mWorld->GetSystemManager()->RegisterSystem<NetRecvSystem>();       // 1. 입력 수신
	mWorld->GetSystemManager()->RegisterSystem<PlayerSystem>();        // 2. 플레이어 입력 → 이동 상태 반영
	mWorld->GetSystemManager()->RegisterSystem<EnemySystem>();         // 3. 적 AI → 이동 상태 반영
	mWorld->GetSystemManager()->RegisterSystem<BeatSystem>();          // 4. 비트 타이밍
	mWorld->GetSystemManager()->RegisterSystem<PlayerInputSystem>();   // 5. 입력 처리
	mWorld->GetSystemManager()->RegisterSystem<MovementSystem>();      // 6. mLocalPosition += v*dt
	mWorld->GetSystemManager()->RegisterSystem<PathFollowSystem>();    // 6-1. PayloadPathData 추종
	mWorld->GetSystemManager()->RegisterSystem<TransformSystem>();     // 7. mWorldMatrix = f(mLocalPosition) ← 이동 후 재계산
	mWorld->GetSystemManager()->RegisterSystem<CameraSystem>();        // 8. 카메라 업데이트
	mWorld->GetSystemManager()->RegisterSystem<MeleeAttackSystem>();   // 9. 근접 공격
	mWorld->GetSystemManager()->RegisterSystem<BulletFireEventSystem>(); // 10. 투사체 발사
	mWorld->GetSystemManager()->RegisterSystem<CollisionSystem>();     // 11. 최신 mWorldMatrix로 충돌 판정
	mWorld->GetSystemManager()->RegisterSystem<InteractionSystem>();   // 11-1. 힐팩/점프대 등 맵 상호작용 트리거 검사
	mWorld->GetSystemManager()->RegisterSystem<SpawnerSystem>();       // 11-2. 주기/이벤트 기반 몬스터 스폰
	mWorld->GetSystemManager()->RegisterSystem<DamageSystem>();        // 12. 데미지/회복 처리
	mWorld->GetSystemManager()->RegisterSystem<PlayerNavValidationSystem>(); // 13. Nav 검증
	mWorld->GetSystemManager()->RegisterSystem<NetSendSystem>();       // 14. 상태 송신 (가장 마지막)

	mSceneId = SceneId::SecondGame;
	shared_ptr<GameMode> gameMode = make_shared<WaveGameMode>();
	SetGameMode(gameMode);
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
