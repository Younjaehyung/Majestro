#pragma once
#include "RenderPass.h"
#include "UIFeature.h"
#include "Entity.h"

class World;
class CameraComponent;

class WorldUIPass
{
public:
    WorldUIPass()  = default;
    ~WorldUIPass() = default;

    void Initialize(World* world);
    void Execute(CameraComponent* camera);
	void SetFeatures(std::vector<shared_ptr<UIFeature>>* features) { mFeatures = features; }

private:
    World* mWorld = nullptr;
	std::vector<shared_ptr<UIFeature>>* mFeatures = nullptr;
};
