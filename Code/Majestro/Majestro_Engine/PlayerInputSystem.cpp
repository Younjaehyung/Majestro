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
	mPhase = SysPhase::Pre;
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


	if (mWorld->HasComponentPool<ChoicePlayerComponent>()) {
		std::vector<Entity> choiceEntitys{ mWorld->GetEntitiesWithComponent<ChoicePlayerComponent>() };
		ChoicePlayerComponent* choicecomponent = mWorld->GetComponent<ChoicePlayerComponent>(choiceEntitys[0]);
		if (INPUT.GetKeyDown(eKeyCode::LEFT))
		{
			choicecomponent->mPlayerType = (choicecomponent->mPlayerType + 2) % 3;
		}
		else if (INPUT.GetKeyDown(eKeyCode::RIGHT))
		{
			choicecomponent->mPlayerType = (choicecomponent->mPlayerType + 1) % 3;
		}
	}

	if (false == mWorld->HasComponentPool<PlayerMovementComponent>())return;
	if (false == mWorld->HasComponentPool<MainPlayerComponent>())return;


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
		if (mainPlayerComponent->GetState() == S_Dash)mainPlayerComponent->mSpeed = mainPlayerComponent->mDashSpeed;
		else mainPlayerComponent->mSpeed = mainPlayerComponent->mRunSpeed;
	}

	movementComponent->mMovingDirection = { 0,0,0 };
	movementComponent->mJump = INPUT.GetKey(eKeyCode::SPACE);
	movementComponent->mDash = INPUT.GetKey(eKeyCode::SHIFT);
	movementComponent->mAttack = INPUT.GetMouseState().LeftDown;
	movementComponent->mSkill1 = INPUT.GetKey(eKeyCode::Q);
	movementComponent->mSkill2 = INPUT.GetKey(eKeyCode::E);
	if (INPUT.GetMouseState().RightDown) {
		cout << "rrrrrrrrr" << endl;
	}
	movementComponent->mSpecial = INPUT.GetMouseState().RightDown;

	if (INPUT.GetKey(eKeyCode::A)) {
		movementComponent->mMovingDirection.x -= 1;
	}
	if (INPUT.GetKey(eKeyCode::W)) {
		movementComponent->mMovingDirection.z += 1;
	}
	if (INPUT.GetKey(eKeyCode::S)) {
		movementComponent->mMovingDirection.z -= 1;
	}
	if (INPUT.GetKey(eKeyCode::D)) {
		movementComponent->mMovingDirection.x += 1;
	}

	if (INPUT.GetKeyDown(eKeyCode::SPACE)) {
		if (beatComponent->mBouns) cout << "Hit Beat!" << endl;
		else cout << "fail" << endl;
		
		//mainPlayerComponent->mFsm.ChangeState(mainPlayerComponent, JumpState::Instance());
		//movementComponent->mJump = true;
		
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


	if (INPUT.IsMouseLookActive()) {
		//attack


		auto mouseDelta = INPUT.GetMouseState().Delta;

		// 미세한 노이즈 제거
		float deltaX = (std::abs(mouseDelta.y) > deadzone) ? (float)mouseDelta.y : 0.0f;
		float deltaY = (std::abs(mouseDelta.x) > deadzone) ? (float)mouseDelta.x : 0.0f;

		movementComponent->targetX += deltaX * sensitivity * mDPI;
		movementComponent->targetY += deltaY * sensitivity * mDPI;

		float alpha = std::lerp(0.0f, 1.0f, dt * lerpFactor); // dt에 비례하도록 수정
		

		movementComponent->currentX = std::lerp(movementComponent->currentX, movementComponent->targetX, alpha);
		movementComponent->currentY = std::lerp(movementComponent->currentY, movementComponent->targetY, alpha);


		movementComponent->mCameraRotationX = movementComponent->currentX;
		movementComponent->mCameraRotationY = movementComponent->currentY;

		// 짐벌락 방지 클램핑 (Pitch)
		movementComponent->mCameraRotationX = std::clamp(movementComponent->mCameraRotationX, -89.0f, 89.0f);
	}


		////screen move
		//movementComponent->mCameraRotationX += (float)INPUT.GetMouseState().Delta.y * dt * mDPI;
		//movementComponent->mCameraRotationY += (float)INPUT.GetMouseState().Delta.x * dt * mDPI;
		INPUT.MouseStateClear();
	

	mainPlayerComponent->Update(dt);

}