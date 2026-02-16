#include "pch.h"
#include "PlayerInputSystem.h"

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
#include "InputComponent.h"
#include "NetEntityComponent.h"

PlayerInputSystem::PlayerInputSystem(World* world) : System(world)
{
}

void PlayerInputSystem::Initialize()
{
}

void PlayerInputSystem::Update(float dt)
{
	if (false == mWorld->HasComponentPool<PlayerMovementComponent>())return;

	std::vector<Entity> entitys{ mWorld->GetEntitiesWithComponent<PlayerMovementComponent>() };

	for (auto& e : entitys) {
		MainPlayerComponent* mainPlayerComponent = mWorld->GetComponent<MainPlayerComponent>(e);
		BeatComponent* beatComponent = mWorld->GetComponent<BeatComponent>(e);
		InputComponent* inputComp = mWorld->GetComponent<InputComponent>(e);
		//NetEntityComponent* netComp = mWorld->GetComponent<NetEntityComponent>(e);

		if (inputComp->MoveX ==0 && inputComp->MoveZ == 0) {
			mainPlayerComponent->mSpeed = 0.f;
			mainPlayerComponent->mFsm.ChangeState(mainPlayerComponent, IdleState::Instance());
		}
		else {
			if (mainPlayerComponent->mDash) mainPlayerComponent->mSpeed = mainPlayerComponent->mDashSpeed;
			else mainPlayerComponent->mSpeed = mainPlayerComponent->mRunSpeed;
		}
		

		//movementComponent->mMovingDirection = { 0,0,0 };

		if (inputComp->MoveX == -1) {
			mainPlayerComponent->mFsm.ChangeState(mainPlayerComponent, WalkState::Instance());
		}
		if (inputComp->MoveZ == 1) {
			mainPlayerComponent->mFsm.ChangeState(mainPlayerComponent, WalkState::Instance());
		}
		if (inputComp->MoveZ == -1) {
			mainPlayerComponent->mFsm.ChangeState(mainPlayerComponent, WalkState::Instance());
		}
		if (inputComp->MoveX == 1) {
			mainPlayerComponent->mFsm.ChangeState(mainPlayerComponent, WalkState::Instance());
		}



		if (inputComp->IsButtonPressed(InputButtons::SPACE)) {
			if (beatComponent->mBouns) cout << "Hit Beat!" << endl;
			else cout << "fail" << endl;

			mainPlayerComponent->mFsm.ChangeState(mainPlayerComponent, JumpState::Instance());
		}
		if (inputComp->IsButtonPressed(InputButtons::SHIFT)) {
			cout << "dash" << endl;
			mainPlayerComponent->mFsm.ChangeState(mainPlayerComponent, DashState::Instance());
		}
		if (inputComp->IsButtonPressed(InputButtons::ATTACK)) {//attack 
			std::cout << "attack!!!" << std::endl;
			mainPlayerComponent->mFsm.ChangeState(mainPlayerComponent, Attack1State::Instance());
		}
		if (inputComp->IsButtonPressed(InputButtons::SPECIAL)) {//attack 
			std::cout << "special" << std::endl;
			mainPlayerComponent->mFsm.ChangeState(mainPlayerComponent, SpecialState::Instance());
		}


		if (inputComp->IsButtonPressed(InputButtons::SKILL1)) {
			std::cout << "skill1" << std::endl;
			mainPlayerComponent->mFsm.ChangeState(mainPlayerComponent, Skill1State::Instance());
		}
		if (inputComp->IsButtonPressed(InputButtons::SKILL2)) {
			std::cout << "skill2" << std::endl;
			mainPlayerComponent->mFsm.ChangeState(mainPlayerComponent, Skill2State::Instance());
		}


		if (inputComp->IsMousePressed(InputMouse::LEFT)) {
			

			//screen move
			//movementComponent->mCameraRotationX += (float)inputComp->Pitch * dt * mDPI;
			//movementComponent->mCameraRotationY += (float)inputComp->Yaw * dt * mDPI;

		}

		mainPlayerComponent->Update(dt);

	}

	
}