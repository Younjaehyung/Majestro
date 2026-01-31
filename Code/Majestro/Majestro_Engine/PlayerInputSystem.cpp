#include "pch.h"
#include "PlayerInputSystem.h"

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

PlayerInputSystem::PlayerInputSystem(World* world) : System(world)
{
}

void PlayerInputSystem::Initialize()
{
}

void PlayerInputSystem::Update(float dt)
{

	//camera setting
	if (false == mWorld->HasComponentPool<MainCameraComponent>())return;

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

	//player move

	std::vector<Entity> entitys{ mWorld->GetEntitiesWithComponent<PlayerMovementComponent>() };
	//std::vector<Entity> entitys{ mWorld->GetEntitiesWithComponents<ControllerComponent, TransformComponent>() };

	PlayerMovementComponent* movementComponent = mWorld->GetComponent<PlayerMovementComponent>(entitys[0]);
	MainPlayerComponent* mainPlayerComponent = mWorld->GetComponent<MainPlayerComponent>(entitys[0]);
	BeatComponent* beatComponent = mWorld->GetComponent<BeatComponent>(entitys[0]);


	if (!INPUT.GetKey(eKeyCode::W) && !INPUT.GetKey(eKeyCode::A) && !INPUT.GetKey(eKeyCode::S) && !INPUT.GetKey(eKeyCode::D)) {
		mainPlayerComponent->mSpeed = 0.f;
		//mainPlayerComponent->mFsm.ChangeState(mainPlayerComponent, IdleState::Instance());
	}
	else {
		if (mainPlayerComponent->GetState() & S_Dash)mainPlayerComponent->mSpeed = mainPlayerComponent->mDashSpeed;
		else mainPlayerComponent->mSpeed = mainPlayerComponent->mRunSpeed;
	}

	movementComponent->mMovingDirection = { 0,0,0 };
	movementComponent->mJump = INPUT.GetKey(eKeyCode::SPACE);
	movementComponent->mAttack1 = INPUT.GetKey(eKeyCode::Q);


	if (INPUT.GetKey(eKeyCode::A)) {
		//mainPlayerComponent->mFsm.ChangeState(mainPlayerComponent, WalkState::Instance());
		movementComponent->mMovingDirection.x -= 1;
	}
	if (INPUT.GetKey(eKeyCode::W)) {
		//mainPlayerComponent->mFsm.ChangeState(mainPlayerComponent, WalkState::Instance());
		movementComponent->mMovingDirection.z += 1;
	}
	if (INPUT.GetKey(eKeyCode::S)) {
		//mainPlayerComponent->mFsm.ChangeState(mainPlayerComponent, WalkState::Instance());
		movementComponent->mMovingDirection.z -= 1;
	}
	if (INPUT.GetKey(eKeyCode::D)) {
		//mainPlayerComponent->mFsm.ChangeState(mainPlayerComponent, WalkState::Instance());
		movementComponent->mMovingDirection.x += 1;
	}



	if (INPUT.GetKeyDown(eKeyCode::SPACE)) {
		if (beatComponent->mBouns) cout << "Hit Beat!" << endl;
		else cout << "fail" << endl;

		
		//mainPlayerComponent->mFsm.ChangeState(mainPlayerComponent, JumpState::Instance());
		//movementComponent->mJump = true;
		
	}
	if (INPUT.GetKeyDown(eKeyCode::SHIFT)) {
		//mainPlayerComponent->mFsm.ChangeState(mainPlayerComponent, DashState::Instance());
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
		movementComponent->mCameraRotationX += (float)INPUT.GetMouseState().Delta.y * dt * mDPI;
		movementComponent->mCameraRotationY += (float)INPUT.GetMouseState().Delta.x * dt * mDPI;
		INPUT.MouseStateClear();
	}

	mainPlayerComponent->Update(dt);

}