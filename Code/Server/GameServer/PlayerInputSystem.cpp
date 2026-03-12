#include "pch.h"
#include "PlayerInputSystem.h"

#include "PlayerSystem.h"
#include "PlayerComponent.h"
#include "CameraComponent.h"
#include "TagComponent.h"
#include "InputManager.h"
#include "TransformSystem.h"
#include "TerrainComponent.h"
#include "BeatComponent.h"
#include "MovementComponent.h"
#include "InputComponent.h"
#include "NetEntityComponent.h"
#include "ServerCore.h"
#include "BulletComponent.h"
#include "EventManager.h"
#include "GameEvents.h"

namespace
{
	void EnqueueAttackEventByCategory(EventManager& eventManager, Entity shooter, BulletType bulletType)
	{
		switch (bulletType)
		{
		case BulletType::BaseAttack:
		case BulletType::BaseSkill1:
		case BulletType::DrumSkill1:
		case BulletType::GuitarSkill1:
			eventManager.Enqueue<EvRangedAttackRequest>({ shooter, bulletType });
			break;

		case BulletType::BaseSkill2:
		case BulletType::DrumSkill2:
		case BulletType::GuitarSkill2:
			eventManager.Enqueue<EvBuffBulletRequest>({ shooter, bulletType });
			break;

		case BulletType::Default:
		default:
			eventManager.Enqueue<EvRangedAttackRequest>({ shooter, bulletType });
			break;
		}
	}

	BulletType ResolveBulletType(uint8 playerType, InputButtons actionButton)
	{
		switch (playerType)
		{
		case 0:
			switch (actionButton)
			{
			case InputButtons::ATTACK: return BulletType::DrumAttack;
			case InputButtons::SKILL1: return BulletType::DrumSkill1;
			case InputButtons::SKILL2: return BulletType::DrumSkill2;
			default: return BulletType::Default;
			}
		case 1:
			switch (actionButton)
			{
			case InputButtons::ATTACK: return BulletType::BaseAttack;
			case InputButtons::SKILL1: return BulletType::BaseSkill1;
			case InputButtons::SKILL2: return BulletType::BaseSkill2;
			default: return BulletType::Default;
			}
		default:
			switch (actionButton)
			{
			case InputButtons::ATTACK: return BulletType::GuitarAttack;
			case InputButtons::SKILL1: return BulletType::GuitarSkill1;
			case InputButtons::SKILL2: return BulletType::GuitarSkill2;
			default: return BulletType::Default;
			}
		}
	}

}

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
		mainPlayerComponent->mPlayerMovingDir.x = inputComp->MoveZ;
		mainPlayerComponent->mPlayerMovingDir.y = inputComp->MoveX;
		if (inputComp->MoveX == 1) {
			mainPlayerComponent->mFsm.ChangeState(mainPlayerComponent, RunRightState::Instance());
		}
		if (inputComp->MoveX == -1) {
			mainPlayerComponent->mFsm.ChangeState(mainPlayerComponent, RunLeftState::Instance());
		}
		if (inputComp->MoveZ == 1) {
			mainPlayerComponent->mFsm.ChangeState(mainPlayerComponent, RunForwardState::Instance());
		}
		if (inputComp->MoveZ == -1) {
			mainPlayerComponent->mFsm.ChangeState(mainPlayerComponent, RunBackwardState::Instance());
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

			if (auto eventManager = mWorld->GetEventManager())
			{
				if (mainPlayerComponent->mPlayerType == 0)
					eventManager->Enqueue<EvMeleeAttackRequest>({ e, MeleeAttackType::DrumAttack });
				else if (mainPlayerComponent->mPlayerType == 1)
					eventManager->Enqueue<EvRangedAttackRequest>({ e, BulletType::BaseAttack });
				else
					eventManager->Enqueue<EvMeleeAttackRequest>({ e, MeleeAttackType::GuitarAttack });
			}
		}
		if (inputComp->IsButtonPressed(InputButtons::RELOAD)) {//attack
			//std::cout << "attack!!!" << std::endl;
			//ActivateBulletAndNotify(e, ResolveBulletType(mainPlayerComponent->mPlayerType, InputButtons::ATTACK));
			mainPlayerComponent->mFsm.ChangeState(mainPlayerComponent, ReRoadState::Instance());
		}
		if (inputComp->IsButtonPressed(InputButtons::SPECIAL)) {//mRhythm change
			//std::cout << "special" << std::endl;
			//mainPlayerComponent->mFsm.ChangeState(mainPlayerComponent, SpecialState::Instance());
		}


		if (inputComp->IsButtonPressed(InputButtons::SKILL1)) {
			//std::cout << "skill1" << std::endl;
			if (auto eventManager = mWorld->GetEventManager())
			{
				const BulletType bulletType = ResolveBulletType(mainPlayerComponent->mPlayerType, InputButtons::SKILL1);
				EnqueueAttackEventByCategory(*eventManager, e, bulletType);
			}
			mainPlayerComponent->mFsm.ChangeState(mainPlayerComponent, Skill1State::Instance());
		}
		if (inputComp->IsButtonPressed(InputButtons::SKILL2)) {
			if (mainPlayerComponent->mPlayerType == 1)
			{
				mainPlayerComponent->mFsm.ChangeState(mainPlayerComponent, DashState::Instance());
			}
			else
			{
				if (auto eventManager = mWorld->GetEventManager())
				{
					const BulletType bulletType = ResolveBulletType(mainPlayerComponent->mPlayerType, InputButtons::SKILL2);
					EnqueueAttackEventByCategory(*eventManager, e, bulletType);
				}
				mainPlayerComponent->mFsm.ChangeState(mainPlayerComponent, Skill2State::Instance());
			}
		}


		if (inputComp->IsMousePressed(InputMouse::LEFT)) {
			

			//screen move
			//movementComponent->mCameraRotationX += (float)inputComp->Pitch * dt * mDPI;
			//movementComponent->mCameraRotationY += (float)inputComp->Yaw * dt * mDPI;

		}

		mainPlayerComponent->Update(dt);

	}

	
}