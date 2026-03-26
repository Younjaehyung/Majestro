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
#include "BeatSystem.h"
#include "GameTimer.h"
#include "BuffComponent.h"

namespace
{
	bool EnqueueAttackEventByCategory(EventManager& eventManager, Entity shooter, SkillType bulletType)
	{
		switch (bulletType)
		{
		case SkillType::DrumAttack:
		case SkillType::DrumSkill1:
		case SkillType::GuitarAttack:
		case SkillType::GuitarSkill1:
			eventManager.Enqueue<EvMeleeAttackRequest>({ shooter, bulletType });
			return true;

		case SkillType::BaseAttack:
		case SkillType::BaseSkill1:
		case SkillType::GuitarAttack_1:
			eventManager.Enqueue<EvRangedAttackRequest>({ shooter, bulletType });
			return true;

		
		case SkillType::DrumSkill2:
		case SkillType::DrumSkill3:
			eventManager.Enqueue<EvBuffRequest>({ shooter, bulletType });
			return true;

		case SkillType::Default:
		default:
			//eventManager.Enqueue<EvRangedAttackRequest>({ shooter, bulletType });
			return true;
		}
	}

	SkillType ResolveSkillType(uint8 playerType, InputButtons actionButton, uint8 rhythm=-1)
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
			case InputButtons::ATTACK: 
				switch(rhythm){
				case 0:  return SkillType::GuitarAttack_1;
				case 1:  return SkillType::GuitarAttack_2;
				case 2:  return SkillType::GuitarAttack_3;
				default: return SkillType::GuitarAttack;
				}
			
			case InputButtons::SKILL1: return SkillType::GuitarSkill1;
			case InputButtons::SKILL2: return SkillType::GuitarSkill2;
			case InputButtons::RELOAD: return SkillType::GuitarSkill3;
			default: return SkillType::Default;
			}
		}
	}

	void EnqueueAmmoChangedIfNeeded(World* world, EventManager& eventManager, Entity playerEntity, int prevAmmo)
	{
		MainPlayerComponent* playerComp = world->GetComponent<MainPlayerComponent>(playerEntity);
		if (!playerComp)
			return;

		if (playerComp->mNowBullet == prevAmmo)
			return;

		eventManager.Enqueue<EvAmmoChanged>({
			playerEntity,
			static_cast<int32>(playerComp->mNowBullet),
			static_cast<int32>(playerComp->mMaxBullet)
			});
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
	auto eventManager = mWorld->GetEventManager();
	if (not eventManager) return;

	std::vector<Entity> entitys{ mWorld->GetEntitiesWithComponent<PlayerMovementComponent>() };

	for (auto& e : entitys) {
		MainPlayerComponent* mainPlayerComponent = mWorld->GetComponent<MainPlayerComponent>(e);
		BeatComponent* beatComponent = mWorld->GetComponent<BeatComponent>(e);
		InputComponent* inputComp = mWorld->GetComponent<InputComponent>(e);
		BuffComponent* buffComp = mWorld->GetComponent<BuffComponent>(e);
		
		//연속행동
		if (mainPlayerComponent->mPendingAction != PendingAction::None)
		{
			
				InputButtons button = InputButtons::ATTACK;
				State<MainPlayerComponent>* pendingState = nullptr;

				switch (mainPlayerComponent->mPendingAction)
				{
				case PendingAction::Attack: button = InputButtons::ATTACK; pendingState = Attack1State::Instance(); break;
				case PendingAction::Skill1: button = InputButtons::SKILL1; pendingState = Skill1State::Instance(); break;
				case PendingAction::Skill2: button = InputButtons::SKILL2; pendingState = Skill2State::Instance(); break;
				case PendingAction::Reload: button = InputButtons::RELOAD; pendingState = ReloadState::Instance(); break;
				default: break;
				}

				const SkillType bulletType = ResolveSkillType(mainPlayerComponent->mPlayerType, button);
				const int prevAmmo = mainPlayerComponent->mNowBullet;

				EnqueueAttackEventByCategory(*eventManager, e, bulletType);
				if (pendingState)
				{
					mainPlayerComponent->mFsm.ChangeState(mainPlayerComponent, pendingState);
					EnqueueAmmoChangedIfNeeded(mWorld, *eventManager, e, prevAmmo);
				}
			

			mainPlayerComponent->mPendingAction = PendingAction::None; // 소비 완료
		}


		if (inputComp->MoveX ==0 && inputComp->MoveZ == 0) {
			mainPlayerComponent->mSpeed = 0.f;
			mainPlayerComponent->mFsm.ChangeState(mainPlayerComponent, IdleState::Instance());
		}
		else {
			if (mainPlayerComponent->mDash) mainPlayerComponent->mSpeed = mainPlayerComponent->mDashSpeed;
			else mainPlayerComponent->mSpeed = mainPlayerComponent->mRunSpeed * buffComp->mMoveSpeedMultiplier;
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
			//cout << "dash" << endl;
			mainPlayerComponent->mFsm.ChangeState(mainPlayerComponent, DashState::Instance());
		}

		auto systemManager = mWorld->GetSystemManager();
		auto* beatSystem = systemManager->GetSystem<BeatSystem>();
		
		const float Beat = beatSystem->mBpmSeconds;
		const float now = GetServerTotalTimeSeconds();

		if (inputComp->IsButtonPressed(InputButtons::ATTACK)) {//attack 
			//std::cout << "attack!!!" << std::endl;
			if (mainPlayerComponent->mNextAttackTime <= now ) {
				if (mainPlayerComponent->mPlayerType == 1 && mainPlayerComponent->mNowBullet > 0) {
					const int prevAmmo = mainPlayerComponent->mNowBullet;
					const SkillType bulletType = ResolveSkillType(mainPlayerComponent->mPlayerType, InputButtons::ATTACK);
					EnqueueAttackEventByCategory(*eventManager, e, bulletType);
					
					mainPlayerComponent->mNextAttackTime = now + Beat * mainPlayerComponent->mAttackCool;
					mainPlayerComponent->mFsm.ChangeState(mainPlayerComponent, Attack1State::Instance());
					EnqueueAmmoChangedIfNeeded(mWorld, *eventManager, e, prevAmmo);
				}
				else if(mainPlayerComponent->mPlayerType == 2) {
					const int prevAmmo = mainPlayerComponent->mNowBullet;
					const SkillType bulletType = ResolveSkillType(mainPlayerComponent->mPlayerType, InputButtons::ATTACK, mainPlayerComponent->mNowBullet);
					EnqueueAttackEventByCategory(*eventManager, e, bulletType);
				
					mainPlayerComponent->mNextAttackTime = now + Beat * mainPlayerComponent->mAttackCool;
					mainPlayerComponent->mFsm.ChangeState(mainPlayerComponent, Attack1State::Instance());
					EnqueueAmmoChangedIfNeeded(mWorld, *eventManager, e, prevAmmo);
				}
				else if (mainPlayerComponent->mPlayerType == 0) {
					
						const SkillType bulletType = ResolveSkillType(mainPlayerComponent->mPlayerType, InputButtons::ATTACK);
						EnqueueAttackEventByCategory(*eventManager, e, bulletType);
					
					mainPlayerComponent->mNextAttackTime = now + Beat * mainPlayerComponent->mAttackCool;
					mainPlayerComponent->mFsm.ChangeState(mainPlayerComponent, Attack1State::Instance());
				}
			}
		}
		if (inputComp->IsButtonPressed(InputButtons::SKILL1)) {
			//std::cout << "skill1" << std::endl;
			if (not mainPlayerComponent->mPlayerType == 1) {
				if (mainPlayerComponent->mNextSkill1Time <= now) {
					const SkillType bulletType = ResolveSkillType(mainPlayerComponent->mPlayerType, InputButtons::SKILL1);
					EnqueueAttackEventByCategory(*eventManager, e, bulletType);
					
					mainPlayerComponent->mNextSkill1Time = now + Beat * mainPlayerComponent->mSkill1Cool;
					mainPlayerComponent->mFsm.ChangeState(mainPlayerComponent, Skill1State::Instance());
				}
			}
			else if (mainPlayerComponent->mNowBullet > 0) {
				if (mainPlayerComponent->mNextSkill1Time <= now) {
					const int prevAmmo = mainPlayerComponent->mNowBullet;
					const SkillType bulletType = ResolveSkillType(mainPlayerComponent->mPlayerType, InputButtons::SKILL1);
					EnqueueAttackEventByCategory(*eventManager, e, bulletType);
					
					mainPlayerComponent->mNextSkill1Time = now + Beat * mainPlayerComponent->mSkill1Cool;
					mainPlayerComponent->mFsm.ChangeState(mainPlayerComponent, Skill1State::Instance());
					EnqueueAmmoChangedIfNeeded(mWorld, *eventManager, e, prevAmmo);
				}
			}
		}
		if (inputComp->IsButtonPressed(InputButtons::SKILL2)) {
			if (mainPlayerComponent->mNextSkill2Time <= now) {
				
					const SkillType bulletType = ResolveSkillType(mainPlayerComponent->mPlayerType, InputButtons::SKILL2);
					EnqueueAttackEventByCategory(*eventManager, e, bulletType);
				
				mainPlayerComponent->mNextSkill2Time = now + Beat * mainPlayerComponent->mSkill2Cool;
				mainPlayerComponent->mFsm.ChangeState(mainPlayerComponent, Skill2State::Instance());
			}
		}

		if (inputComp->IsButtonPressed(InputButtons::RELOAD)) {
			//std::cout << "reroad" << std::endl;
			if (mainPlayerComponent->mNextReloadTime <= now) {
				const int prevAmmo = mainPlayerComponent->mNowBullet;
				const SkillType bulletType = ResolveSkillType(mainPlayerComponent->mPlayerType, InputButtons::RELOAD);
				EnqueueAttackEventByCategory(*eventManager, e, bulletType);
				mainPlayerComponent->mNextReloadTime = now + Beat * mainPlayerComponent->mReloadCool;
				mainPlayerComponent->mFsm.ChangeState(mainPlayerComponent, ReloadState::Instance());
				EnqueueAmmoChangedIfNeeded(mWorld, *eventManager, e, prevAmmo);
			}
		}
		if (inputComp->IsButtonPressed(InputButtons::SPECIAL)) {//mRhythm change - R click
			if (beatComponent->mBouns) cout << "Hit Beat!" << endl;
			else cout << "fail" << endl;

			if (mainPlayerComponent->mNextRythmChangeTime <= now) {
				mainPlayerComponent->mNextRythmChangeTime = now + 0.1f;
				mainPlayerComponent->mNextRhythm = (mainPlayerComponent->mNextRhythm + 1) % 3;
				if (mainPlayerComponent->mRhythm != mainPlayerComponent->mNextRhythm) mainPlayerComponent->mHasQueuedRhythmChange = true;

			}

			//std::cout << "special" << std::endl;
			//mainPlayerComponent->mFsm.ChangeState(mainPlayerComponent, SpecialState::Instance());
		}

		//cout << "bullet: " << mainPlayerComponent->mNowBullet << endl;
		if (inputComp->IsMousePressed(InputMouse::LEFT)) {
			

			//screen move
			//movementComponent->mCameraRotationX += (float)inputComp->Pitch * dt * mDPI;
			//movementComponent->mCameraRotationY += (float)inputComp->Yaw * dt * mDPI;

		}

		mainPlayerComponent->Update(dt);

	}

	
}