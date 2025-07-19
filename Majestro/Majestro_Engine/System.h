#pragma once
#include <vector>

class World;

class System{
public:
    virtual ~System() = default;
    virtual void Update(float deltaTime) = 0;   //Logic��
    virtual void Update() = 0;                  //Render��
    virtual void Initialize() {}
    virtual void Shutdown() {}

protected:
    System(World* world) : mWorld(world) {}

    World* mWorld;
};
