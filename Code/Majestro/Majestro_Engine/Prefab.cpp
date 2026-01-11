#include "pch.h"
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

Prefab::Prefab() : Object(OBJECT_TYPE::PREFAB)
{
}

Prefab::~Prefab()
{
}


PlayerPrefab::PlayerPrefab(shared_ptr<World> world)
{
	Entity osw = world->CreateEntity();

	TransformComponent t{};
	Entity testCamera = world->CreateEntity();
	world->AddComponent<MainCameraComponent>(testCamera);
	world->AddComponent<CameraComponent>(testCamera);
	world->AddComponent<TransformComponent>(testCamera, t);
	world->AddComponent<CameraTypeComponent>(testCamera, osw.GetID(), THREE_FPS);

	shared_ptr<Mesh> phereMesh = RESOURCEMANAGER.Get<Mesh>(L"Rudwig_mBody");
		
	std::vector<shared_ptr<Material>> material2s;

	shared_ptr<Material> material2 = RESOURCEMANAGER.Get<Material>(L"Rudwig_aIdle_0010");
		

	material2s.push_back(material2);
	t.mLocalPosition = { 0.f, 0.f, 10.f };
	t.mLocalScale = { 10.f, 10.f, 10.f };

	vector<shared_ptr<Animator>> anmators0;
	anmators0.push_back(RESOURCEMANAGER.Get<Animator>(L"Armature|Rudwig_aIdle_001"));
	anmators0.push_back(RESOURCEMANAGER.Get<Animator>(L"Armature|Rudwig_aWalk_001"));
	anmators0.push_back(RESOURCEMANAGER.Get<Animator>(L"Armature|Rudwig_aRun_001"));
	anmators0.push_back(RESOURCEMANAGER.Get<Animator>(L"Armature|Rudwig_aJump_001"));
	anmators0.push_back(RESOURCEMANAGER.Get<Animator>(L"Armature|Rudwig_aRun_001"));//dash


	world->AddComponent<ControllerComponent>(osw, t);
	world->AddComponent<MainPlayerComponent>(osw, "../Resources/Json/TestJson.json", anmators0);
	world->AddComponent<TransformComponent>(osw, t);
	world->AddComponent<RenderComponent>(osw, phereMesh, material2s);
	world->AddComponent<AnimationComponent>(osw, anmators0);
	world->AddComponent<BeatComponent>(osw);
	world->AddComponent<GravityComponent>(osw);
	world->AddComponent<MovementComponent>(osw);

}

PlayerPrefab::~PlayerPrefab()
{
}

