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

		// 사망
		if (mainPlayerComponent && mainPlayerComponent->IsDeathActive())
		{
			mainPlayerComponent->mSpeed = 0.f;
			mainPlayerComponent->mHasMoveInput = false;
			continue;
		}

		auto systemManager = mWorld->GetSystemManager();
		auto* beatSystem = systemManager->GetSystem<BeatSystem>();
		const float Beat = beatSystem->mBpmSeconds;
		const float now  = GetServerTotalTimeSeconds();

		//연속행동
		if (mainPlayerComponent->mPendingAction != PendingAction::None)
		{
			InputButtons btn = InputButtons::ATTACK;
			switch (mainPlayerComponent->mPendingAction)
			{
			case PendingAction::Attack: btn = InputButtons::ATTACK; break;
			case PendingAction::Skill1: btn = InputButtons::SKILL1; break;
			case PendingAction::Skill2: btn = InputButtons::SKILL2; break;
			case PendingAction::Reload: btn = InputButtons::RELOAD; break;
			default: break;
			}
			TryFireAction(e, mainPlayerComponent, *eventManager, btn, now, Beat);
			mainPlayerComponent->mPendingAction = PendingAction::None; // 소비 완료
		}


		mainPlayerComponent->mHasMoveInput =
			std::abs(inputComp->MoveX) > 0.01f ||
			std::abs(inputComp->MoveZ) > 0.01f;

		const bool hasActionState =
			mainPlayerComponent->GetReplicatedActionState() != static_cast<uint8>(ReplicatedActionState::None);
		const bool canUseHorizontalInput = mainPlayerComponent->CanUseHorizontalInput();

		if (!mainPlayerComponent->mHasMoveInput) {
			mainPlayerComponent->mSpeed = 0.f;
			if (!hasActionState)
				mainPlayerComponent->mFsm.ChangeState(mainPlayerComponent, IdleState::Instance());
		}
		else {
			if (mainPlayerComponent->mDash) mainPlayerComponent->mSpeed = mainPlayerComponent->mDashSpeed;
			else if (canUseHorizontalInput) mainPlayerComponent->mSpeed = mainPlayerComponent->mRunSpeed * buffComp->mMoveSpeedMultiplier;
			else mainPlayerComponent->mSpeed = 0.f;
		}
		

		//movementComponent->mMovingDirection = { 0,0,0 };
		mainPlayerComponent->mPlayerMovingDir.x = inputComp->MoveZ;
		mainPlayerComponent->mPlayerMovingDir.y = inputComp->MoveX;
		if (mainPlayerComponent->mDash) {
			inputComp->MoveZ = 1;
			inputComp->MoveX = 0;
		}
		else if (canUseHorizontalInput && !hasActionState) {
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
		}


		if (inputComp->IsButtonPressed(InputButtons::SPACE)) {
			mainPlayerComponent->mFsm.ChangeState(mainPlayerComponent, JumpState::Instance());
		}
		if (inputComp->IsButtonPressed(InputButtons::SHIFT)) {
			//cout << "dash" << endl;
			mainPlayerComponent->mFsm.ChangeState(mainPlayerComponent, DashState::Instance());
		}

		if (inputComp->IsButtonPressed(InputButtons::ATTACK)) {
			TryFireAction(e, mainPlayerComponent, *eventManager, InputButtons::ATTACK, now, Beat);
		}
		if (inputComp->IsButtonPressed(InputButtons::SKILL1)) {
			TryFireAction(e, mainPlayerComponent, *eventManager, InputButtons::SKILL1, now, Beat);
		}
		if (inputComp->IsButtonPressed(InputButtons::SKILL2)) {
			TryFireAction(e, mainPlayerComponent, *eventManager, InputButtons::SKILL2, now, Beat);
		}

		if (inputComp->IsButtonPressed(InputButtons::RELOAD)) {
			TryFireAction(e, mainPlayerComponent, *eventManager, InputButtons::RELOAD, now, Beat);
		}
		if (inputComp->IsButtonPressed(InputButtons::SPECIAL)) {//mRhythm change - R click
			mainPlayerComponent->mFsm.ChangeState(mainPlayerComponent, RhythmChangeState::Instance());
			//if (beatComponent->mBouns) cout << "Hit Beat!" << endl;
			//else cout << "fail" << endl;

			/*if (mainPlayerComponent->mNextRythmChangeTime <= now) {
				mainPlayerComponent->mNextRythmChangeTime = now + 0.1f;
				mainPlayerComponent->mNextRhythm = (mainPlayerComponent->mNextRhythm + 1) % 4;
				if (mainPlayerComponent->mRhythm != mainPlayerComponent->mNextRhythm) mainPlayerComponent->mHasQueuedRhythmChange = true;

			}*/

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


bool PlayerInputSystem::EnqueueAttackEventByCategory(EventManager& eventManager, Entity shooter, SkillType bulletType)
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
	case SkillType::GuitarAttack_2:
	case SkillType::GuitarAttack_3:
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

SkillType PlayerInputSystem::ResolveSkillType(uint8 playerType, InputButtons actionButton, uint8 rhythm)
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
			switch (rhythm) {
			case 1:  return SkillType::GuitarAttack_1;
			case 2:  return SkillType::GuitarAttack_2;
			case 3:  return SkillType::GuitarAttack_3;
			default: return SkillType::GuitarAttack;
			}

		case InputButtons::SKILL1: return SkillType::GuitarSkill1;
		case InputButtons::SKILL2: return SkillType::GuitarSkill2;
		case InputButtons::RELOAD: return SkillType::GuitarSkill3;
		default: return SkillType::Default;
		}
	}
}

void PlayerInputSystem::EnqueueAmmoChangedIfNeeded(World* world, EventManager& eventManager, Entity playerEntity, int prevAmmo)
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

bool PlayerInputSystem::TryFireAction(Entity e, MainPlayerComponent* mp, EventManager& em,
                                      InputButtons button, float now, float Beat)
{
	if (mp == nullptr) return false;

	float* nextTimePtr = nullptr;
	float  cool = 0.f;
	State<MainPlayerComponent>* nextState = nullptr;
	bool   needsAmmo = false;

	switch (button)
	{
	case InputButtons::ATTACK:
		nextTimePtr = &mp->mNextAttackTime;
		cool        = mp->mAttackCool;
		nextState   = Attack1State::Instance();
		needsAmmo   = (mp->mPlayerType == Ibanix); // Fanthor 탄0 기본공격 유지
		break;
	case InputButtons::SKILL1:
		nextTimePtr = &mp->mNextSkill1Time;
		cool        = mp->mSkill1Cool;
		nextState   = Skill1State::Instance();
		needsAmmo   = (mp->mPlayerType == Ibanix); // 기존 룰
		break;
	case InputButtons::SKILL2:
		nextTimePtr = &mp->mNextSkill2Time;
		cool        = mp->mSkill2Cool;
		nextState   = Skill2State::Instance();
		needsAmmo   = false;
		break;
	case InputButtons::RELOAD:
		nextTimePtr = &mp->mNextReloadTime;
		cool        = mp->mReloadCool;
		nextState   = ReloadState::Instance();
		needsAmmo   = false;
		break;
	default:
		return false;
	}

	if (nextTimePtr == nullptr || nextState == nullptr) return false;
	if (*nextTimePtr > now) return false;
	if (needsAmmo && mp->mNowBullet <= 0) return false;

	if (button == InputButtons::SKILL1 &&
		mp->mPlayerType == Rudwig)
	{
		if (mp->mStateThrew)
		{
			mp->mStateThrew = false;
			mp->mFsm.ChangeState(mp, nextState);
			return true;
		}

		if (mp->GetState() == S_Skill1)
			return false;
	}

	// 박자 약화: Fanthor ATTACK만 rhythm 사용. 박자 빗나가면 rhythm=0 폴백.
	uint8 rhythm = 0;
	if (button == InputButtons::ATTACK
		&& mp->mPlayerType == Fanthor
		&& mp->mNowBullet > 0)
	{
		BeatComponent* bc = mWorld->GetComponent<BeatComponent>(e);
		rhythm = (bc != nullptr && bc->mBouns) ? mp->mRhythm : 0;
	}

	const int prevAmmo = mp->mNowBullet;
	const SkillType bulletType = ResolveSkillType(mp->mPlayerType, button, rhythm);
	EnqueueAttackEventByCategory(em, e, bulletType);

	switch (button)
	{
	case InputButtons::ATTACK:
		if (mp->mPlayerType == Ibanix || mp->mPlayerType == Fanthor)
			mp->mNowBullet = (std::max)(0, mp->mNowBullet - 1);
		break;
	case InputButtons::SKILL1:
		if (mp->mPlayerType == Ibanix)
			mp->mNowBullet = (std::max)(0, mp->mNowBullet - 1);
		break;
	default:
		break;
	}

	if (!(button == InputButtons::SKILL1 && mp->mPlayerType == Rudwig && !mp->mStateThrew))
		mp->mFsm.ChangeState(mp, nextState);

	if (button == InputButtons::SKILL1 && mp->mPlayerType == Rudwig)
		mp->mStateThrew = true;
	*nextTimePtr = now + Beat * cool;
	EnqueueAmmoChangedIfNeeded(mWorld, em, e, prevAmmo);
	return true;
}
