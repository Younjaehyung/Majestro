#include "pch.h"
#include "PlayerSystem.h"
#include "PlayerComponent.h"
#include "TransformComponent.h"
#include "CameraComponent.h"
#include "TagComponent.h"
#include "InputManager.h"
#include "TransformSystem.h"
#include "TerrainComponent.h"
#include "BeatComponent.h"
#include "MovementComponent.h"


PlayerSystem::PlayerSystem(World* world) : System(world)
{
}

void PlayerSystem::Initialize()
{
}

void PlayerSystem::Update(float dt)
{
	std::vector<Entity> entitys{ mWorld->GetEntitiesWithComponents<ControllerComponent, TransformComponent>() };

	for(auto & entity : entitys)
	{
		MainPlayerComponent* mainPlayerComponent = mWorld->GetComponent<MainPlayerComponent>(entity);
		mainPlayerComponent->Update(dt);
	}
	
}
