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

	std::vector<Entity> playerEntitys= mWorld->GetEntitiesWithComponent<CameraTypeComponent>() ;
	for (auto& entity : playerEntitys) {

		CameraTypeComponent* mainPlayer = mWorld->GetComponent<CameraTypeComponent>(entity);
		CameraComponent* cameraComponent = mWorld->GetComponent<CameraComponent>(entity);
		MainPlayerComponent* mainPlayerComponent = mWorld->GetComponent<MainPlayerComponent>(mainPlayer->mTargetID);



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
			const float yawDelta = MathUtils::Wrap180Degrees(moveCom->mCameraRotationY - tfCom->mLocalRotationE.y);
			const float clampedYaw = std::clamp(yawDelta, -kAimYawLimitDeg, kAimYawLimitDeg);

			animCom->mEnableAimOffset = aimActive;
			animCom->mAimPitch = aimActive ? (pitchDeg * deg2rad) : 0.f;
			animCom->mAimYaw   = aimActive ? (clampedYaw * deg2rad) : 0.f;
		}

	}
}

