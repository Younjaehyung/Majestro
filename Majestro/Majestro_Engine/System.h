#pragma once
#include <vector>

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

    World* mWorld;
};
