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
    // INPUT
    RegisterSystem<PlayerInputSystem>();

	// NETWORK
    RegisterSystem<NetRecvSystem>(mWorld->GetNetIdMap());
    RegisterSystem<NetSendSystem>();

	// GAME
    RegisterSystem<AnimationSystem>();
    RegisterSystem<CameraSystem>();
    
    
    RegisterSystem<EnemySystem>();
    //RegisterSystem<PlayerSystem>();
    RegisterSystem<TransformSystem>();
    RegisterSystem<MovementSystem>();
   

    RegisterSystem<AudioVisualizerSystem>();

    RegisterSystem<UITransformSystem>();
    RegisterSystem<UIUpdateSystem>();

    RegisterSystem<AudioSystem>();
    RegisterSystem<BeatSystem>();
    RegisterSystem<NetInterpolationSystem>();
	// RENDER
    RegisterSystem<RenderSystem>();
    RegisterSystem<UIRenderSystem>();

#ifdef _IMGUI
	RegisterSystem<IMGUIRenderSystem>();
#else
#endif

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

void SystemManager::Render()
{
    RenderPhase(SysPhase::Render);
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

        // 결정성을 위해: 큐 대신 "indeg 0 집합"을 정렬하는 방식이 더 안정적이지만
        // 여기서는 간단히, pop 전에 input Order 정렬을 기반으로 어느 정도 안정성 유지.
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

        // 사이클 감지: out.size != input.size
        if (out.size() != input.size())
        {
            // 의존성 사이클이 있으면 "Order 정렬"만 적용한 input을 반환하거나,
            // 여기서 assert/예외로 터뜨려 조기 발견하는 걸 권장.
            // 상용 엔진에서는 대개 개발 빌드에서 강하게 터뜨림.
            throw std::runtime_error("System dependency cycle detected in phase");
        }

        // out을 Order 기준으로 한번 더 안정 정렬할 필요는 없음(그래프가 순서를 규정)
        return out;
    
}

void SystemManager::RebuildScheduleIfDirty()
{ 

    if (mDirtySchedule) return;
    mDirtySchedule = false;

    for (size_t i = 0; i < (size_t)SysPhase::Count; ++i)
    {
        // 1) 기본 Order 정렬
        auto& vec = mPhaseSystems[i];
        std::sort(vec.begin(), vec.end(), [](System* a, System* b) {
            if (a->GetOrder() != b->GetOrder()) return a->GetOrder() < b->GetOrder();
            // 동일 Order면 typeid name으로 안정 정렬 (결정성 확보)
            return std::string(a->GetName()) < std::string(b->GetName());
            });

        // 2) (옵션) 의존성 기반 토폴로지 정렬
        //    - 의존성을 안 쓰면 아래 함수는 그냥 vec를 그대로 반환
        vec = TopoSortPhase(static_cast<SysPhase>(i), vec);
    }
}