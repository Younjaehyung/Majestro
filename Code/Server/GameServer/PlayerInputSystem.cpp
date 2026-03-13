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
	bool EnqueueAttackEventByCategory(EventManager& eventManager, Entity shooter, SkillType bulletType)
	{
		switch (bulletType)
		{
		case SkillType::DrumAttack:
		case SkillType::GuitarAttack:
			eventManager.Enqueue<EvMeleeAttackRequest>({ shooter, bulletType });
			return true;

		case SkillType::BaseAttack:
		case SkillType::BaseSkill1:
		case SkillType::DrumSkill1:
		case SkillType::GuitarSkill1:
			eventManager.Enqueue<EvRangedAttackRequest>({ shooter, bulletType });
			return true;

		
		case SkillType::DrumSkill2:
		case SkillType::GuitarSkill2:
			eventManager.Enqueue<EvBuffBulletRequest>({ shooter, bulletType });
			return true;

		case SkillType::Default:
		default:
			//eventManager.Enqueue<EvRangedAttackRequest>({ shooter, bulletType });
			return true;
		}
	}

	SkillType ResolveSkillType(uint8 playerType, InputButtons actionButton)
	{
		switch (playerType)
		{
		case 0:
			switch (actionButton)
			{
			case InputButtons::ATTACK: return SkillType::DrumAttack;
			case InputButtons::SKILL1: return SkillType::DrumSkill1;
			case InputButtons::SKILL2: return SkillType::DrumSkill2;
			case InputButtons::RELOAD: return SkillType::DrumSkill3;
			default: return SkillType::Default;
			}
		case 1:
			switch (actionButton)
			{
			case InputButtons::ATTACK: return SkillType::BaseAttack;
			case InputButtons::SKILL1: return SkillType::BaseSkill1;
			case InputButtons::SKILL2: return SkillType::BaseSkill2;
			case InputButtons::RELOAD: return SkillType::BaseSkill3;
			default: return SkillType::Default;
			}
		default:
			switch (actionButton)
			{
			case InputButtons::ATTACK: return SkillType::GuitarAttack;
			case InputButtons::SKILL1: return SkillType::GuitarSkill1;
			case InputButtons::SKILL2: return SkillType::GuitarSkill2;
			case InputButtons::RELOAD: return SkillType::GuitarSkill3;
			default: return SkillType::Default;
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
		
		//연속행동
		if (mainPlayerComponent->mPendingAction != PendingAction::None)
		{
			if (auto eventManager = mWorld->GetEventManager())
			{
				InputButtons button = InputButtons::ATTACK;

				switch (mainPlayerComponent->mPendingAction)
				{
				case PendingAction::Attack: button = InputButtons::ATTACK; break;
				case PendingAction::Skill1: button = InputButtons::SKILL1; break;
				case PendingAction::Skill2: button = InputButtons::SKILL2; break;
				case PendingAction::Reload: button = InputButtons::RELOAD; break;
				default: break;
				}

				const SkillType bulletType = ResolveSkillType(mainPlayerComponent->mPlayerType, button);

				EnqueueAttackEventByCategory(*eventManager, e, bulletType);
			}

			mainPlayerComponent->mPendingAction = PendingAction::None; // 소비 완료
		}


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
			mainPlayerComponent->mFsm.ChangeState(mainPlayerComponent, JumpState::Instance());
		}
		if (inputComp->IsButtonPressed(InputButtons::SHIFT)) {
			cout << "dash" << endl;
			mainPlayerComponent->mFsm.ChangeState(mainPlayerComponent, DashState::Instance());
		}


		if (inputComp->IsButtonPressed(InputButtons::ATTACK)) {//attack 
			//std::cout << "attack!!!" << std::endl;
			if (auto eventManager = mWorld->GetEventManager())
			{
				const SkillType bulletType = ResolveSkillType(mainPlayerComponent->mPlayerType, InputButtons::ATTACK);
				EnqueueAttackEventByCategory(*eventManager, e, bulletType);
			}
			mainPlayerComponent->mFsm.ChangeState(mainPlayerComponent, Attack1State::Instance());
		}
		if (inputComp->IsButtonPressed(InputButtons::SKILL1)) {
			//std::cout << "skill1" << std::endl;
			if (auto eventManager = mWorld->GetEventManager())
			{
				const SkillType bulletType = ResolveSkillType(mainPlayerComponent->mPlayerType, InputButtons::SKILL1);
				EnqueueAttackEventByCategory(*eventManager, e, bulletType);
			}
			mainPlayerComponent->mFsm.ChangeState(mainPlayerComponent, Skill1State::Instance());
		}
		if (inputComp->IsButtonPressed(InputButtons::SKILL2)) {
			if (auto eventManager = mWorld->GetEventManager())
			{
				const SkillType bulletType = ResolveSkillType(mainPlayerComponent->mPlayerType, InputButtons::SKILL2);
				EnqueueAttackEventByCategory(*eventManager, e, bulletType);
			}
			mainPlayerComponent->mFsm.ChangeState(mainPlayerComponent, Skill2State::Instance());
		}

		if (inputComp->IsButtonPressed(InputButtons::RELOAD)) {
			//std::cout << "reroad" << std::endl;
			if (auto eventManager = mWorld->GetEventManager())
			{
				const SkillType bulletType = ResolveSkillType(mainPlayerComponent->mPlayerType, InputButtons::RELOAD);
				EnqueueAttackEventByCategory(*eventManager, e, bulletType);
			}
			mainPlayerComponent->mFsm.ChangeState(mainPlayerComponent, ReRoadState::Instance());
		}
		if (inputComp->IsButtonPressed(InputButtons::SPECIAL)) {//mRhythm change - R click
			if (beatComponent->mBouns) cout << "Hit Beat!" << endl;
			else cout << "fail" << endl;

			//std::cout << "special" << std::endl;
			//mainPlayerComponent->mFsm.ChangeState(mainPlayerComponent, SpecialState::Instance());
		}


		if (inputComp->IsMousePressed(InputMouse::LEFT)) {
			

			//screen move
			//movementComponent->mCameraRotationX += (float)inputComp->Pitch * dt * mDPI;
			//movementComponent->mCameraRotationY += (float)inputComp->Yaw * dt * mDPI;

		}

		mainPlayerComponent->Update(dt);

	}

	
}