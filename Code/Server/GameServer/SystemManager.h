#pragma once
#include "System.h"
#include "EventManager.h"

class SystemManager
{
public:
    SystemManager(World* world);
    ~SystemManager();

    void Update(float deltaTime);

    void Shutdown();

    template<typename T, typename... Args>
    T* RegisterSystem(Args&&... args);

    template<typename T>
    T* GetSystem();

private:
    World* mWorld;
    std::shared_ptr<EventManager> mEventManager = make_shared<EventManager>();
    std::vector<std::unique_ptr<System>> mSystems;

    std::unordered_map<size_t, System*> mSystemMap;

    std::vector<std::unique_ptr<System>> mAwakeSystems;
    std::vector<std::unique_ptr<System>> mStartSystems;
    std::vector<std::unique_ptr<System>> mUpdateSystems;
    std::vector<std::unique_ptr<System>> mLateUpdateSystems;
    std::vector<std::unique_ptr<System>> mFinalUpdateSystems;
    std::vector<std::unique_ptr<System>> mRenderSystems;

};

template<typename T, typename... Args>
T* SystemManager::RegisterSystem(Args&&... args) {
    static_assert(std::is_base_of_v<System, T>, "T must inherit from BaseSystem");

    auto system = std::make_unique<T>(mWorld, std::forward<Args>(args)...);
    T* systemPtr = system.get();

    mSystems.emplace_back(std::move(system));
    mSystemMap[typeid(T).hash_code()] = systemPtr;

    systemPtr->Initialize();
    return systemPtr;
}

template<typename T>
T* SystemManager::GetSystem() {
    size_t typeHash = typeid(T).hash_code();
    auto it = mSystemMap.find(typeHash);

    if (it != mSystemMap.end()) {
        return static_cast<T*>(it->second);
    }

    return nullptr;
}