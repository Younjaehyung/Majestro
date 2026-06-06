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
#include "RenderComponent.h"
#include "MovementComponent.h"
#include "AnimationComponent.h"
#include "MovementSystem.h"

#include "MathUtils.h"

PlayerSystem::PlayerSystem(World* world) : System(world)
{
}

void PlayerSystem::Initialize()
{
}

std::vector<std::type_index> PlayerSystem::After() const
{
	return { typeid(MovementSystem) };
}

void PlayerSystem::Update(float dt)
{
	if (false == mWorld->HasComponentPool<MainPlayerComponent>())return;
	if (false == mWorld->HasComponentPool<LocalPlayerComponent>())return;

	/*std::vector<Entity> entitys{ mWorld->GetEntitiesWithComponents<ControllerComponent, TransformComponent>() };
	if (entitys.empty())return;
	MainPlayerComponent* mainPlayerComponent = mWorld->GetComponent<MainPlayerComponent>(entitys[0]);
	mainPlayerComponent->Update(dt);*/

	std::vector<Entity> playerEntitys= mWorld->GetEntitiesWithComponent<CameraTypeComponent>() ;
	for (auto& entity : playerEntitys) {

		CameraTypeComponent* mainPlayer = mWorld->GetComponent<CameraTypeComponent>(entity);
		CameraComponent* cameraComponent = mWorld->GetComponent<CameraComponent>(entity);
		MainPlayerComponent* mainPlayerComponent = mWorld->GetComponent<MainPlayerComponent>(mainPlayer->mTargetID);


		//if(mainPlayerComponent->mStatePacket == S_Attack1)
		//{
		//	mainPlayerComponent->mShakeCameraTime += dt;


		//	cameraComponent->mFov = lerp(cameraComponent->mFov, (103.f / 2.0f) + 0.2f, mainPlayerComponent->mDashTime);
		//	
		//}
		//else
		//{
		//	mainPlayerComponent->mShakeCameraTime = 0.f;
		//	cameraComponent->
		//}



		if(mainPlayerComponent->mLowerState == static_cast<int>(ReplicatedMovementMode::Dashing))
		{
			mainPlayerComponent->mDashTime += dt;
			cameraComponent->mFov = lerp(cameraComponent->mFov, (103.f / 2.0f) + 0.2f, mainPlayerComponent->mDashTime);

		}
		else
		{
			mainPlayerComponent->mDashTime = 0.f;
			cameraComponent->mFov = 103.f / 2.0f;
		}

		// AimOffset / TurnInPlace — AnimationComponent에 pitch/yaw 주입
		AnimationComponent* animCom = mWorld->GetComponent<AnimationComponent>(mainPlayer->mTargetID);
		PlayerMovementComponent* moveCom = mWorld->GetComponent<PlayerMovementComponent>(mainPlayer->mTargetID);
		TransformComponent*      tfCom   = mWorld->GetComponent<TransformComponent>(mainPlayer->mTargetID);
		if (animCom && moveCom && tfCom)
		{
			const bool battleAimState =
				mainPlayerComponent->mUpperState == static_cast<int>(ReplicatedActionState::Aim) ||
				mainPlayerComponent->mUpperState == static_cast<int>(ReplicatedActionState::Attack1) ||
				mainPlayerComponent->mUpperState == static_cast<int>(ReplicatedActionState::Attack2) ||
				mainPlayerComponent->mUpperState == static_cast<int>(ReplicatedActionState::Skill1) ||
				mainPlayerComponent->mUpperState == static_cast<int>(ReplicatedActionState::Skill2) ||
				mainPlayerComponent->mUpperState == static_cast<int>(ReplicatedActionState::Special);

			const bool aimActive = INPUT.IsMouseLookActive() || battleAimState || moveCom->mAttack || moveCom->mSpecial;

			const float deg2rad = 0.01745329252f;
			constexpr float kAimPitchLimitDeg = 60.f;
			constexpr float kAimYawLimitDeg = 60.f;

			const float pitchDeg = std::clamp(moveCom->mCameraRotationX, -kAimPitchLimitDeg, kAimPitchLimitDeg);
			// AimYaw는 절대 카메라 yaw가 아니라 현재 몸 yaw 기준의 카메라 방향 차이.
			const float yawDelta = Wrap180Degrees(moveCom->mCameraRotationY - tfCom->mLocalRotationE.y);
			const float clampedYaw = std::clamp(yawDelta, -kAimYawLimitDeg, kAimYawLimitDeg);

			animCom->mEnableAimOffset = aimActive;
			animCom->mAimPitch = aimActive ? (pitchDeg * deg2rad) : 0.f;
			animCom->mAimYaw   = aimActive ? (clampedYaw * deg2rad) : 0.f;
		}

		
		// 공격 상태 진입 시 카메라 쉐이크 트리거
		if (mainPlayerComponent->mUpperState != mainPlayerComponent->mPrevStatePacket ||
			mainPlayerComponent->mStateSequence != mainPlayerComponent->mPrevStateSequence)
		{
			uint32 entityID = entity.GetID();
			int32  currState = mainPlayerComponent->mUpperState;

			//mainPlayerComponent->mPlayerType = 0 1 2


			// TriggerShake(float magnitude, float duration, float frequency)
			switch (mainPlayerComponent->mPlayerType) 
			{
			case Rudwig:
				if (currState)
				{
					bool bAttackState = (currState == static_cast<int>(ReplicatedActionState::Attack1) || currState == static_cast<int>(ReplicatedActionState::Attack2)
						|| currState == static_cast<int>(ReplicatedActionState::Skill1) || currState == static_cast<int>(ReplicatedActionState::Skill2)
						|| currState == static_cast<int>(ReplicatedActionState::Special));
					if (bAttackState)
						mainPlayer->TriggerShake(1.5f, 0.05f, 20.f);
					std::cout << "ATTACK!@" << std::endl;
				}
				break;
			case Ibanix:
				if (currState)
				{
					bool bAttackState = (currState == static_cast<int>(ReplicatedActionState::Attack1) || currState == static_cast<int>(ReplicatedActionState::Attack2)
						|| currState == static_cast<int>(ReplicatedActionState::Skill1) || currState == static_cast<int>(ReplicatedActionState::Skill2)
						|| currState == static_cast<int>(ReplicatedActionState::Special));
					if (bAttackState)
						mainPlayer->TriggerShake(0.5f, 0.05f, 20.f);

				}
				break;
			case Fanthor: 
				if (currState)
				{
					bool bAttackState = (currState == static_cast<int>(ReplicatedActionState::Attack1) || currState == static_cast<int>(ReplicatedActionState::Attack2)
						|| currState == static_cast<int>(ReplicatedActionState::Skill1) || currState == static_cast<int>(ReplicatedActionState::Skill2)
						|| currState == static_cast<int>(ReplicatedActionState::Special));
					if (bAttackState)
						mainPlayer->TriggerShake(0.5f, 0.05f, 20.f);

				}
				break;
			}
		}

	}
}


