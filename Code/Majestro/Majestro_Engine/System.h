#pragma once
#include <vector>

class EventManager;
class World;

class System{
public:
    virtual ~System() = default;
    virtual void Update(float deltaTime) {}   //Logic��
    virtual void Update() {}                  //Render��
    virtual void Initialize() {}
    virtual void Shutdown() {}

protected:
    System(World* world) : mWorld(world) {}
	System(World* world, EventManager* event) : mWorld(world), mEventManager(event){} // for future use with args

    World* mWorld;
    EventManager* mEventManager = nullptr;
};
