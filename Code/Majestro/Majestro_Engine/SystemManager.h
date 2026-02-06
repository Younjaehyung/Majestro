#pragma once
#include "System.h"
#include "EventManager.h"


class SystemManager
{
public:
    SystemManager(World* world);
    ~SystemManager();

    void Update(float deltaTime);
	void RunPhase(SysPhase phase, float deltaTime);
	void RenderPhase(SysPhase phase);

    void WorldBegin();
    void WorldEnd();

    void Render();

    void Shutdown();

    template<typename T, typename... Args>
    T* RegisterSystem(Args&&... args);


    template<typename T>
    T* UnRegisterSystem();


    template<typename T>
    T* GetSystem();


    // phase별로만 의존성 그래프를 만든다.
    std::vector<System*> TopoSortPhase(SysPhase phase, const std::vector<System*>& input);
    void RebuildScheduleIfDirty();
private:
    World* mWorld = nullptr;
   
    std::vector<std::unique_ptr<System>> mSystems;
    std::unordered_map<std::type_index, System*> mSystemMap;

    // phase별 실행 리스트(포인터)
    std::vector<System*> mPhaseSystems[(size_t)SysPhase::Count];

    bool mDirtySchedule = true;
};




template<typename T, typename... Args>
T* SystemManager::RegisterSystem(Args&&... args) {
    const std::type_index key = std::type_index(typeid(T));
    if (mSystemMap.contains(key))
        throw std::runtime_error("RegisterSystem: already registered");

    auto sys = std::make_unique<T>(mWorld,  std::forward<Args>(args)...);
    T* raw = sys.get();

    mSystemMap.emplace(key, raw);
    mSystems.emplace_back(std::move(sys));

    raw->Initialize();

    // Phase 리스트에 포인터 등록
    mPhaseSystems[(size_t)raw->GetPhase()].push_back(raw);

    mDirtySchedule = true;
    return raw;
}

template<typename T>
T* SystemManager::UnRegisterSystem() //? 되는 코드인가
{
    const std::type_index key = std::type_index(typeid(T));
    auto it = mSystemMap.find(key);
    if (it == mSystemMap.end()) return;

    System* victim = it->second;
    mSystemMap.erase(it);

    // mAllSystems에서 제거
    std::erase_if(mSystems, [&](const std::unique_ptr<System>& p) {
        return p.get() == victim;
        });

    // Phase 리스트에서도 제거
    for (auto& vec : mPhaseSystems)
        std::erase(vec, victim);

    mDirtySchedule = true;
}

template<typename T>
T* SystemManager::GetSystem() {
    const std::type_index key = std::type_index(typeid(T));
    auto it = mSystemMap.find(key);
    if (it == mSystemMap.end()) return nullptr;
    return static_cast<T*>(it->second);
}