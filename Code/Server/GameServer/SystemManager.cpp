#include "pch.h"
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


    //GetSystem<NetRecvSystem>()->Update(deltaTime);
    //GetSystem<EnemySystem>()->Update(deltaTime);
    //GetSystem<PlayerInputSystem>()->Update(deltaTime);
    //GetSystem<MovementSystem>()->Update(deltaTime);
    //GetSystem<TransformSystem>()->Update(deltaTime);
    //GetSystem<CameraSystem>()->Update(deltaTime);
    //GetSystem<PlayerSystem>()->Update(deltaTime);
    //GetSystem<CollisionSystem>()->Update(deltaTime);

    //GetSystem<BeatSystem>()->Update(deltaTime);
    //GetSystem<NetSendSystem>()->Update(deltaTime);



SystemManager::SystemManager(World* world) : mWorld(world)
{
    RegisterSystem<NetRecvSystem>();
    RegisterSystem<NetSendSystem>();
    RegisterSystem<CameraSystem>();
    RegisterSystem<TransformSystem>();
    RegisterSystem<PlayerSystem>();
    RegisterSystem<EnemySystem>();
    RegisterSystem<BeatSystem>();
    RegisterSystem<MovementSystem>();
    RegisterSystem<PlayerInputSystem>();
    RegisterSystem<MeleeAttackSystem>();
    RegisterSystem<BulletFireEventSystem>();
    RegisterSystem<CollisionSystem>();
    RegisterSystem<DamageSystem>();
    RegisterSystem<PlayerNavValidationSystem>();

}

SystemManager::~SystemManager()
{
}

void SystemManager::Update(float deltaTime) {
    mWorld->GetEventManager()->SwapBuffers();

    RunPhase(SysPhase::Pre, deltaTime);
    RunPhase(SysPhase::Sim, deltaTime);
    RunPhase(SysPhase::Post, deltaTime);

}

void SystemManager::Shutdown()
{
    /*  for (auto& s : mSystems)
          s->Shutdown();*/
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
    for (auto& [node, d] : indeg)
        if (d == 0) q.push(node);

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

    if (mDirtySchedule) return;
    mDirtySchedule = false;

    for (size_t i = 0; i < (size_t)SysPhase::Count; ++i)
    {
        // 기본 Order 정렬
        auto& vec = mPhaseSystems[i];
        std::sort(vec.begin(), vec.end(), [](System* a, System* b) {
            if (a->GetOrder() != b->GetOrder()) return a->GetOrder() < b->GetOrder();
            // 동일 Order면 typeid name으로 안정 정렬
            return std::string(a->GetName()) < std::string(b->GetName());
            });

        // 의존성 기반 토폴로지 정렬
        //  - 의존성을 안 쓰면 아래 함수는 그냥 vec를 그대로 반환
        vec = TopoSortPhase(static_cast<SysPhase>(i), vec);
    }
}