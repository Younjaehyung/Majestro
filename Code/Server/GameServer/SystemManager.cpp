#include "pch.h"
#include "SystemManager.h"
#include "CameraSystem.h"
#include "TransformSystem.h"
#include "PlayerSystem.h"
#include "BeatSystem.h"
#include "MovementSystem.h"
#include "NetRecvSystem.h"
#include "NetSendSystem.h"
#include "PlayerInputSystem.h"

SystemManager::SystemManager(World* world) : mWorld(world) 
{

    RegisterSystem<NetRecvSystem>();
    RegisterSystem<NetSendSystem>();
    RegisterSystem<CameraSystem>();
    RegisterSystem<TransformSystem>();
    RegisterSystem<PlayerSystem>();
    RegisterSystem<BeatSystem>();
    RegisterSystem<MovementSystem>();
    RegisterSystem<PlayerInputSystem>();
    
}

SystemManager::~SystemManager()
{
}

void SystemManager::Update(float deltaTime) {
    for (auto& sys : mAwakeSystems)        sys->Update(deltaTime);
    for (auto& sys : mStartSystems)        sys->Update(deltaTime);
    for (auto& sys : mUpdateSystems)       sys->Update(deltaTime);
    for (auto& sys : mLateUpdateSystems)   sys->Update(deltaTime);
    for (auto& sys : mFinalUpdateSystems)  sys->Update(deltaTime);

    GetSystem<NetRecvSystem>()->Update(deltaTime);
    GetSystem<PlayerInputSystem>()->Update(deltaTime);
    GetSystem<MovementSystem>()->Update(deltaTime);
    GetSystem<TransformSystem>()->Update(deltaTime);
    GetSystem<CameraSystem>()->Update(deltaTime);
    GetSystem<PlayerSystem>()->Update(deltaTime);
   
    GetSystem<BeatSystem>()->Update(deltaTime);
    GetSystem<NetSendSystem>()->Update(deltaTime);
    

    
}


void SystemManager::Shutdown() {
    for (auto& sys : mAwakeSystems)         sys->Shutdown();
    for (auto& sys : mStartSystems)         sys->Shutdown();
    for (auto& sys : mUpdateSystems)        sys->Shutdown();
    for (auto& sys : mLateUpdateSystems)    sys->Shutdown();
    for (auto& sys : mFinalUpdateSystems)   sys->Shutdown();

    mAwakeSystems.clear();
    mStartSystems.clear();
    mUpdateSystems.clear();
    mFinalUpdateSystems.clear();

    mSystemMap.clear();
}