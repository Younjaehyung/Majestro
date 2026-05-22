#pragma once
#include "System.h"
#include "World.h"

class UIMainMenuSystem : public System
{
public:
    UIMainMenuSystem(World* w) : System(w) { mPhase = SysPhase::Post; }

    std::vector<std::type_index> After()  const override;
    std::vector<std::type_index> Before() const override;

    void Update(float dt) override;

private:
    bool DetectAnyInputDown() const;
    void SetEntitiesVisible(const std::vector<class Entity>& es, bool visible);
};
