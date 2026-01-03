#include "pch.h"
#include "SystemManager.h"
//
//#include "CameraSystem.h"
//
//#include "TransformSystem.h"
//
//#include "PlayerSystem.h"
//
//#include "UIUpdateSystem.h"


SystemManager::SystemManager(World* world) : mWorld(world) 
{

  //  RegisterSystem<CameraSystem>();
   
	//RegisterSystem<UITransformSystem>();

   // RegisterSystem<TransformSystem>();
  //  RegisterSystem<PlayerSystem>();

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

  /*  GetSystem<TransformSystem>()->Update(deltaTime);
    GetSystem<CameraSystem>()->Update(deltaTime);


    GetSystem<PlayerSystem>()->Update(deltaTime);
    GetSystem<UITransformSystem>()->Update(deltaTime);*/
    
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