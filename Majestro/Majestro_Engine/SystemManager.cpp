#include "pch.h"
#include "SystemManager.h"
#include "RenderSystem.h"
#include "CameraSystem.h"

SystemManager::SystemManager(World* world) : mWorld(world) 
{
    RegisterSystem<RenderSystem>();
    RegisterSystem<CameraSystem>();

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

    GetSystem<CameraSystem>()->Update();
}

void SystemManager::Render() {
    for (auto& sys : mRenderSystems)        sys->Update();
    GetSystem<RenderSystem>()->Update();
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