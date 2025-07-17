#pragma once
#include <vector>

class World;

class System{
public:
    virtual ~System() = default;
    virtual void Update(float deltaTime) = 0;
    virtual void Initialize() {}
    virtual void Shutdown() {}

protected:
    System(World* world) : mWorld(world) {}

    World* mWorld;
};
