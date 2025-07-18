#pragma once
#include <vector>

class World;

class System{
public:
    virtual ~System() = default;
    virtual void Update(float deltaTime) = 0;   //Logic¿ë
    virtual void Update() = 0;                  //Render¿ë
    virtual void Initialize() {}
    virtual void Shutdown() {}

protected:
    System(World* world) : mWorld(world) {}

    World* mWorld;
};
