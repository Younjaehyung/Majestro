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
//#include "Camera.h"
//
//#include "ConstantBuffer.h"
//#include "Light.h"
//#include "Resources.h"


void Scene::Initialize()
{
	//PlayerPrefab p{ mWorld.get()};
	PrefabFactory::RegisterAllPrefabs();

	TerrainPrefab terrain{ mWorld.get()};

	//LoadCollisionJson(L"..\\Resources\\Json\\Map001_CRX.json");
	
	mWorld->Initialize();
}

void Scene::Update(float deltaTime)
{
	mWorld->Update(deltaTime);
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




