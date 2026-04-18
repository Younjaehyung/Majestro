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
	mWorld->Update(deltaTime);
	mGameMode->Update(deltaTime);
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
			shared_ptr<Mesh> data = RESOURCEMANAGER.LoadMCubeMesh();

			std::vector<Vec3> localVertices;
			for (const Vertex& v : data->GetVertexBuffer())
				localVertices.push_back(v.pos);

			BoundingOrientedBox localObb;
			BoundingOrientedBox::CreateFromPoints(localObb, localVertices.size(), localVertices.data(), sizeof(Vec3));

			Entity entity = mWorld->CreateEntity();

			// mLocalOBB = localObb (단위 박스)
			// PhysicsWorld::Initialize에서 mLocalOBB.Transform(worldMatrix) → mWorldOBB 올바르게 계산됨
			BoxColliderComponent& boxCollider = mWorld->AddComponent<BoxColliderComponent>(entity, localObb, inst.worldMtx);

			// worldMatrix를 반드시 설정해야 PhysicsWorld::Initialize가 올바른 위치로 변환함
			TransformComponent transform{};
			transform.mWorldMatrix = inst.worldMtx;
			TransformComponent& trans = mWorld->AddComponent<TransformComponent>(entity, transform);
			trans.mIsStatic = true;
			mWorld->AddComponent<StaticComponent>(entity);
			++i;
			std::cout << i << std::endl;
		}
	}
	catch (const std::exception& e)
	{
		std::cerr << "Load failed: " << e.what() << "\n";
	}
}

// halfExtents = 트리거 영역 반경 / valueA,valueB = kind별 파라미터 / cooldown = 0이면 지속형.
Entity Scene::SpawnInteractable(World* world,
	uint8 kind,
	const Vec3& position,
	const Vec3& halfExtents,
	float valueA,
	float valueB,
	float cooldown,
	bool  oneShot,
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

	return e;
}

void FirstScene::Initialize()
{
	//PlayerPrefab p{ mWorld.get()};
	PrefabFactory::RegisterAllPrefabs();

	TerrainPrefab terrain{ mWorld.get() };

	//LoadCollisionJson(L"..\\Resources\\Json\\Map001_CRX.json");
	mWorld->Initialize();
	mWorld->GetSystemManager()->RegisterSystem<NetRecvSystem>();       // 1. 입력 수신
	mWorld->GetSystemManager()->RegisterSystem<PlayerSystem>();
	mWorld->GetSystemManager()->RegisterSystem<BuffSystem>();          // 5. 버프 틱/만료 처리// 2. 플레이어 입력 이동 상태 반영
	mWorld->GetSystemManager()->RegisterSystem<EnemySystem>();         // 3. 적 AI → 이동 상태 반영
	mWorld->GetSystemManager()->RegisterSystem<BeatSystem>();          // 4. 비트 타이밍
	mWorld->GetSystemManager()->RegisterSystem<PlayerInputSystem>();   // 5. 입력 처리
	mWorld->GetSystemManager()->RegisterSystem<MovementSystem>();      // 6. mLocalPosition += v*dt
	mWorld->GetSystemManager()->RegisterSystem<TransformSystem>();     // 7. mWorldMatrix = f(mLocalPosition)  이동 후 재계산
	mWorld->GetSystemManager()->RegisterSystem<CameraSystem>();        // 8. 카메라 업데이트
	mWorld->GetSystemManager()->RegisterSystem<MeleeAttackSystem>();   // 9. 근접 공격
	mWorld->GetSystemManager()->RegisterSystem<BulletFireEventSystem>(); // 10. 투사체 발사
	mWorld->GetSystemManager()->RegisterSystem<CollisionSystem>();     // 11. 최신 mWorldMatrix로 충돌 판정
	mWorld->GetSystemManager()->RegisterSystem<InteractionSystem>();   // 11-1. 힐팩/점프대 등 맵 상호작용 트리거 검사
	mWorld->GetSystemManager()->RegisterSystem<DamageSystem>();        // 12. 데미지/회복 처리
	mWorld->GetSystemManager()->RegisterSystem<PlayerNavValidationSystem>(); // 13. Nav 검증
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
		/*valueB(전방 임펄스)=*/10.0f,
		/*cooldown=*/0.5f,
		/*oneShot=*/false);

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
	mWorld->GetSystemManager()->RegisterSystem<TransformSystem>();     // 7. mWorldMatrix = f(mLocalPosition) ← 이동 후 재계산
	mWorld->GetSystemManager()->RegisterSystem<CameraSystem>();        // 8. 카메라 업데이트
	mWorld->GetSystemManager()->RegisterSystem<MeleeAttackSystem>();   // 9. 근접 공격
	mWorld->GetSystemManager()->RegisterSystem<BulletFireEventSystem>(); // 10. 투사체 발사
	mWorld->GetSystemManager()->RegisterSystem<CollisionSystem>();     // 11. 최신 mWorldMatrix로 충돌 판정
	mWorld->GetSystemManager()->RegisterSystem<InteractionSystem>();   // 11-1. 힐팩/점프대 등 맵 상호작용 트리거 검사
	mWorld->GetSystemManager()->RegisterSystem<DamageSystem>();        // 12. 데미지/회복 처리
	mWorld->GetSystemManager()->RegisterSystem<PlayerNavValidationSystem>(); // 13. Nav 검증
	mWorld->GetSystemManager()->RegisterSystem<NetSendSystem>();       // 14. 상태 송신 (가장 마지막)

	mSceneId = SceneId::SecondGame;
}

void LobbyScene::Initialize()
{
	mWorld->Initialize();
	mSceneId = SceneId::Lobby;
}

void VictoryScene::Initialize()
{
	mWorld->Initialize();
	mSceneId = SceneId::VGame;
}

void LoseScene::Initialize()
{
	mWorld->Initialize();
	mWorld->GetSystemManager()->RegisterSystem<NetRecvSystem>();       // 1. 입력 수신

	mWorld->GetSystemManager()->RegisterSystem<NetSendSystem>();       // 14. 상태 송신 (가장 마지막)
	mSceneId = SceneId::LGame;
}
