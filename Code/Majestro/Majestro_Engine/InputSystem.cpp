#include "pch.h"
#include "InputSystem.h"

#include "PlayerSystem.h"
#include "Engine.h"
#include "PlayerComponent.h"
#include "TransformComponent.h"
#include "CameraComponent.h"
#include "TagComponent.h"
#include "InputManager.h"
#include "TransformSystem.h"
#include "TerrainComponent.h"
#include "BeatComponent.h"
#include "MovementComponent.h"

InputSystem::InputSystem(World* world) : System(world)
{
}

void InputSystem::Initialize()
{
}

void InputSystem::Update(float dt)
{

	std::vector<Entity> mainCameraEntitys{ mWorld->GetEntitiesWithComponent<MainCameraComponent>() };
	CameraTypeComponent* cameraTypeComponent = mWorld->GetComponent<CameraTypeComponent>(mainCameraEntitys[0]);

	if (INPUT.GetKeyDown(eKeyCode::F1)) {
		cameraTypeComponent->mPlayMode = ONE_FPS;
	}
	else if (INPUT.GetKeyDown(eKeyCode::F2)) {
		cameraTypeComponent->mPlayMode = THREE_FPS;
	}
	else if (INPUT.GetKeyDown(eKeyCode::F3)) {
		cameraTypeComponent->mPlayMode = THREE_RPG;
	}
	else if (INPUT.GetKeyDown(eKeyCode::F4)) {
		cameraTypeComponent->mPlayMode = MAIN_CAMERA;
	}
}