#include "pch.h"
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


PlayerSystem::PlayerSystem(World* world) : System(world)
{
}

void PlayerSystem::Initialize()
{
}

void PlayerSystem::Update(float dt)
{
	std::vector<Entity> entitys{ mWorld->GetEntitiesWithComponents<ControllerComponent, TransformComponent>() };
	
	MainPlayerComponent* mainPlayerComponent = mWorld->GetComponent<MainPlayerComponent>(entitys[0]);
	mainPlayerComponent->Update(dt);
}

void PlayerSystem::Input(float dt, PlayerMovementComponent* movementComponent, MainPlayerComponent* mainPlayerComponent, bool beatHit)
{


	if (!INPUT.GetKey(eKeyCode::W) && !INPUT.GetKey(eKeyCode::A) && !INPUT.GetKey(eKeyCode::S) && !INPUT.GetKey(eKeyCode::D)) {
		mainPlayerComponent->mSpeed = 0.f;
		mainPlayerComponent->mFsm.ChangeState(mainPlayerComponent, IdleState::Instance());
	}
	else {
		if (mainPlayerComponent->GetState() & S_Dash)mainPlayerComponent->mSpeed = mainPlayerComponent->mDashSpeed;
		else mainPlayerComponent->mSpeed = mainPlayerComponent->mRunSpeed;
	}

	movementComponent->mMovingDirection = { 0,0,0 };

	if (INPUT.GetKey(eKeyCode::A)) {
		mainPlayerComponent->mFsm.ChangeState(mainPlayerComponent, WalkState::Instance());
		movementComponent->mMovingDirection.x -=1;
		}
	if (INPUT.GetKey(eKeyCode::W)) {
		mainPlayerComponent->mFsm.ChangeState(mainPlayerComponent, WalkState::Instance());
		movementComponent->mMovingDirection.z += 1;
		}
	if (INPUT.GetKey(eKeyCode::S)) {
		mainPlayerComponent->mFsm.ChangeState(mainPlayerComponent, WalkState::Instance());
		movementComponent->mMovingDirection.z -= 1;
		}
	if (INPUT.GetKey(eKeyCode::D)) {
		mainPlayerComponent->mFsm.ChangeState(mainPlayerComponent, WalkState::Instance());
		movementComponent->mMovingDirection.x += 1;
		}



	if (INPUT.GetKeyDown(eKeyCode::SPACE)) {
		if (beatHit) cout << "Hit Beat!" << endl;
		else cout << "fail" << endl;

		mainPlayerComponent->mFsm.ChangeState(mainPlayerComponent, JumpState::Instance());
	}
	if (INPUT.GetKeyDown(eKeyCode::SHIFT)) {
		mainPlayerComponent->mFsm.ChangeState(mainPlayerComponent, DashState::Instance());
	}


	if (INPUT.GetKey(eKeyCode::Q)) {
		movementComponent->mMovingDirection.y -= 1;
	}
	if (INPUT.GetKey(eKeyCode::E)) {
		movementComponent->mMovingDirection.y += 1;
	}


	if (INPUT.GetMouseState().LeftDown) {
		//attack

		//screen move
		movementComponent->mCameraRotationX += (float)INPUT.GetMouseState().Delta.y * dt * DPI;
		movementComponent->mCameraRotationY += (float)INPUT.GetMouseState().Delta.x * dt * DPI;
		INPUT.MouseStateClear();
	}
	
}