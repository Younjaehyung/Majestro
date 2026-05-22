#pragma once
#include "System.h"
#include "World.h"

class MainMenuCameraSystem : public System
{
public:
    MainMenuCameraSystem(World* w) : System(w) { mPhase = SysPhase::Post; }

    std::vector<std::type_index> Before() const override;

    void Update(float dt) override;
};
