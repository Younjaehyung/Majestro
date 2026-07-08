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

#include "NetRecvSystem.h"
#include "NetSendSystem.h"
#include "PlayerInputSystem.h"
#include "EnemySystem.h"
#include "CollisionSystem.h"
#include "NetInterpolationSystem.h"
#include "AudioVisualizerSystem.h"

SystemManager::SystemManager(World* world) : mWorld(world) 
{
 //   // INPUT
 //   RegisterSystem<PlayerInputSystem>();

	//// NETWORK
 //   RegisterSystem<NetRecvSystem>(mWorld->GetNetIdMap());
 //   RegisterSystem<NetSendSystem>();

	//// GAME
 //   RegisterSystem<AnimationSystem>();
 //   RegisterSystem<CameraSystem>();
 //   
 //   
 //   RegisterSystem<EnemySystem>();
 //   //RegisterSystem<PlayerSystem>();
 //   RegisterSystem<TransformSystem>();
 //   RegisterSystem<MovementSystem>();
 //  

 //   RegisterSystem<AudioVisualizerSystem>();

 //   RegisterSystem<UITransformSystem>();
 //   RegisterSystem<UIUpdateSystem>();

 //   RegisterSystem<AudioSystem>();
 //   RegisterSystem<BeatSystem>();
 //   RegisterSystem<NetInterpolationSystem>();
	//// RENDER
 //   RegisterSystem<RenderSystem>();
 //   RegisterSystem<UIRenderSystem>();

#ifdef _IMGUI
	RegisterSystem<IMGUIRenderSystem>();
#else
#endif

}

SystemManager::~SystemManager()
{
}

void SystemManager::Update(float deltaTime) 
{

    RebuildScheduleIfDirty();

    mWorld->GetEventManager()->SwapBuffers();

    RunPhase(SysPhase::Pre, deltaTime);
    RunPhase(SysPhase::Sim, deltaTime);
    RunPhase(SysPhase::Post, deltaTime);

}

void SystemManager::Render()
{
    RenderPhase(SysPhase::Render);
}

void SystemManager::Shutdown()
{
    for (auto& vec : mPhaseSystems)
        vec.clear();

    mSystemMap.clear();
    mSystems.clear();
    mDirtySchedule = true;
}

void SystemManager::WorldBegin()
{
    for (auto& s : mSystems)
        s->OnWorldBegin();
}

void SystemManager::RunPhase(SysPhase phase, float deltaTime)
{
    auto& vec = mPhaseSystems[(size_t)phase];
    for (auto& s : vec)
        s->Update(deltaTime);
}

void SystemManager::RenderPhase(SysPhase phase)
{
    auto& vec = mPhaseSystems[(size_t)phase];
    for (auto& s : vec)
        s->Update();
}



void SystemManager::WorldEnd()
{
    for (auto& s : mSystems)
        s->OnWorldEnd();
}


std::vector<System*> SystemManager::TopoSortPhase(SysPhase phase, const std::vector<System*>& input)
{
   
        // Phase 내 시스템이 0~1개면 그대로
        if (input.size() <= 1)
            return input;

        // 타입 -> System*
        std::unordered_map<std::type_index, System*> typeToSys;
        typeToSys.reserve(input.size());
        for (System* s : input)
            typeToSys.emplace(std::type_index(typeid(*s)), s);

        // 그래프 구성: u -> v (u가 v보다 먼저)
        std::unordered_map<System*, std::vector<System*>> edges;
        std::unordered_map<System*, int> indeg;

        for (System* s : input)
        {
            edges[s]; // ensure
            indeg[s] = 0;
        }

        auto addEdge = [&](System* u, System* v)
            {
                if (!u || !v || u == v) return;
                edges[u].push_back(v);
                indeg[v] += 1;
            };

        // Before / After를 모두 지원
        for (System* s : input)
        {
            // s.Before(): s -> target
            for (auto& t : s->Before())
            {
                auto it = typeToSys.find(t);
                if (it != typeToSys.end())
                    addEdge(s, it->second);
            }

            // s.After(): target -> s
            for (auto& t : s->After())
            {
                auto it = typeToSys.find(t);
                if (it != typeToSys.end())
                    addEdge(it->second, s);
            }
        }

        // Kahn
        std::queue<System*> q;

        for (System* s : input)
            if (indeg[s] == 0) q.push(s);

        std::vector<System*> out;
        out.reserve(input.size());

        while (!q.empty())
        {
            System* u = q.front();
            q.pop();
            out.push_back(u);

            for (System* v : edges[u])
            {
                if (--indeg[v] == 0)
                    q.push(v);
            }
        }

        if (out.size() != input.size())
        {
            throw std::runtime_error("System dependency cycle detected in phase");
        }

        return out;
    
}

void SystemManager::RebuildScheduleIfDirty()
{ 
    if (!mDirtySchedule) return;
    mDirtySchedule = false;

    for (size_t i = 0; i < (size_t)SysPhase::Count; ++i)
    {

        auto& vec = mPhaseSystems[i];

        std::stable_sort(vec.begin(), vec.end(), [](System* a, System* b) {
            return a->GetOrder() < b->GetOrder();
            });


        vec = TopoSortPhase(static_cast<SysPhase>(i), vec);
    }
}
