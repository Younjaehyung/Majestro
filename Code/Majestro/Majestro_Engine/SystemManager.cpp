#include "pch.h"
#include "SystemManager.h"
#include "RenderSystem.h"
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
#include "EffectSystem.h"
#include "NetRecvSystem.h"
#include "NetSendSystem.h"
#include "PlayerInputSystem.h"
#include "EnemySystem.h"
#include "CollisionSystem.h"
#include "NetInterpolationSystem.h"

SystemManager::SystemManager(World* world) : mWorld(world) 
{
    RegisterSystem<NetRecvSystem>(mEventManager.get(), mWorld->GetNetIdMap());
    RegisterSystem<NetSendSystem>(mEventManager.get());

    RegisterSystem<CameraSystem>();
    RegisterSystem<RenderSystem>();
    RegisterSystem<UIRenderSystem>();
	RegisterSystem<UITransformSystem>();

    RegisterSystem<AnimationSystem>();
    RegisterSystem<AudioSystem>();
    RegisterSystem<TransformSystem>();
    RegisterSystem<PlayerSystem>();
    RegisterSystem<BeatSystem>();
    RegisterSystem<MovementSystem>();
    RegisterSystem<EffectSystem>();
    RegisterSystem<PlayerInputSystem>();
    RegisterSystem<EnemySystem>();
    RegisterSystem<CollisionSystem>();
    RegisterSystem<NetInterpolationSystem>();

#ifdef _IMGUI
	RegisterSystem<IMGUIRenderSystem>();
#else
#endif
}

SystemManager::~SystemManager()
{
}

void SystemManager::Update(float deltaTime) {
    //for (auto& sys : mAwakeSystems)        sys->Update(deltaTime);
    //for (auto& sys : mStartSystems)        sys->Update(deltaTime);
    //for (auto& sys : mUpdateSystems)       sys->Update(deltaTime);
    //for (auto& sys : mLateUpdateSystems)   sys->Update(deltaTime);
    //for (auto& sys : mFinalUpdateSystems)  sys->Update(deltaTime);

	NetUpdate(deltaTime);

    PreUpdate(deltaTime);
	
	PostUpdate(deltaTime);

}

void SystemManager::NetUpdate(float deltaTime)
{
    GetSystem<NetRecvSystem>()->Update(deltaTime);
    GetSystem<NetSendSystem>()->Update(deltaTime);
}

void SystemManager::PreUpdate(float deltaTime)
{
    mEventManager->BeginPhase(EventPhase::Pre);
    GetSystem<PlayerInputSystem>()->Update(deltaTime);
    GetSystem<CollisionSystem>()->Update(deltaTime);
   // GetSystem<MovementSystem>()->Update(deltaTime);
    GetSystem<NetInterpolationSystem>()->Update(deltaTime);
    GetSystem<CameraSystem>()->Update(deltaTime);
    GetSystem<EnemySystem>()->Update(deltaTime);

}

void SystemManager::PostUpdate(float deltaTime)
{
    mEventManager->BeginPhase(EventPhase::Post);
    GetSystem<TransformSystem>()->Update(deltaTime);
    GetSystem<PlayerSystem>()->Update(deltaTime);
    GetSystem<UITransformSystem>()->Update(deltaTime);
    GetSystem<BeatSystem>()->Update(deltaTime);
    GetSystem<AudioSystem>()->Update(deltaTime);
    GetSystem<EffectSystem>()->Update(deltaTime);
    GetSystem<AnimationSystem>()->Update(deltaTime);
}

void SystemManager::Render() {
    for (auto& sys : mRenderSystems)        sys->Update();
    GetSystem<RenderSystem>()->Update();
    GetSystem<EffectSystem>()->Update();
    GetSystem<UIRenderSystem>()->Update();
   
#ifdef _IMGUI
	//GetSystem<IMGUIRenderSystem>()->Update();
#else
#endif

}

void SystemManager::Shutdown() {
    for (auto& sys : mAwakeSystems)         sys->Shutdown();
    for (auto& sys : mStartSystems)         sys->Shutdown();
    for (auto& sys : mUpdateSystems)        sys->Shutdown();
    for (auto& sys : mLateUpdateSystems)    sys->Shutdown();
    for (auto& sys : mFinalUpdateSystems)   sys->Shutdown();
    for (auto& sys : mRenderSystems)        sys->Shutdown();



    mAwakeSystems.clear();
    mStartSystems.clear();
    mUpdateSystems.clear();
    mFinalUpdateSystems.clear();
    mRenderSystems.clear();

    mSystemMap.clear();
}