#include "pch.h"
#include "MovementSystem.h"
#include "TransformSystem.h"
#include "TransformComponent.h"
#include "TerrainComponent.h"
#include "GravityComponent.h"
#include "MovementComponent.h"
#include "NetTransformComponent.h"
#include "CameraComponent.h"
#include "TagComponent.h"
#include "PlayerComponent.h"
#include "BulletComponent.h"
#include "VfxComponent.h"

#include "MathUtils.h"
	
MovementSystem::MovementSystem(World* world) : System(world)
{

}



void MovementSystem::Update(float dt) {
	if (false == mWorld->HasComponentPool<MainCameraComponent>())return;
	if (false == mWorld->HasComponentPool<PlayerMovementComponent>())return;

	//terrain
	//auto terrainEntities = mWorld->GetEntitiesWithComponent<TerrainComponent>();
	//TerrainComponent* terrainComponent = mWorld->GetComponent<TerrainComponent>(terrainEntities[0]);


	//main player movement
	std::vector<Entity> mainCameraEntitys{ mWorld->GetEntitiesWithComponent<MainCameraComponent>() };
	if (mainCameraEntitys.empty())return;
	CameraTypeComponent* cameraTypeComponent = mWorld->GetComponent<CameraTypeComponent>(mainCameraEntitys[0]);

	std::vector<Entity> playerEntitys{ mWorld->GetEntitiesWithComponents<PlayerMovementComponent, LocalPlayerComponent>() };
	for (auto& entity : playerEntitys) {
		PlayerMovementComponent* movementComponent = mWorld->GetComponent<PlayerMovementComponent>(entity);
		TransformComponent* transformComponent = mWorld->GetComponent<TransformComponent>(entity);
		MainPlayerComponent* mainPlayerComponent = mWorld->GetComponent<MainPlayerComponent>(entity);
		NetTransformComponent* netTransformComponent = mWorld->GetComponent<NetTransformComponent>(entity);

		const bool isLobbyScene = (mWorld->GetSceneId() == SceneId::Lobby);
		const bool hasMoveInput = (movementComponent->mMovingDirection.LengthSquared() > 0.0001f);

		if (cameraTypeComponent->mPlayMode == ONE_FPS || cameraTypeComponent->mPlayMode == THREE_FPS) {
			Vec3 forward = transformComponent->GetLook();
			Vec3 right = transformComponent->GetRight();

			// WASD 입력
			float ix = movementComponent->mMovingDirection.x;  // A/D  (-1 ~ 1)
			float iy = movementComponent->mMovingDirection.z;  // W/S   (-1 ~ 1)

			// 로컬 입력 방향을 월드 방향으로 변환
			Vec3 desired = forward * iy + right * ix;

			// 정규화
			if (desired.LengthSquared() > 0.0001f)
				desired.Normalize();

			transformComponent->mLocalPosition += desired * dt * mainPlayerComponent->mSpeed;

			if (cameraTypeComponent->mPlayMode == THREE_FPS && !isLobbyScene) {
				// 정지 중에는 상체 AimYaw를 남기고, TIP 상태가 시작된 뒤에는 stopAngle까지 계속 회전.
				UpdateBodyYawForAim(movementComponent, transformComponent, dt, hasMoveInput);
			}
			else {
				// ONE_FPS + Lobby
				if (!isLobbyScene || hasMoveInput) {
					transformComponent->mLocalRotationE.y = movementComponent->mCameraRotationY;
					ResetTurnInPlace(movementComponent);
				}
			}

			netTransformComponent->mStartPosition = transformComponent->mLocalPosition;
			netTransformComponent->mStartRotation.y = transformComponent->mLocalRotationE.y;
			netTransformComponent->mStartRotation.x = transformComponent->mLocalRotationE.x;
		}
		else if (cameraTypeComponent->mPlayMode == THREE_RPG) {
			// F3 RPG: 카메라 yaw 기준으로 이동, 캐릭터는 진행 방향으로 회전 (카메라/조작 분리)
			const float camYaw = DirectX::XMConvertToRadians(movementComponent->mCameraRotationY);
			Matrix yawMat = Matrix::CreateFromQuaternion(
				Quaternion::CreateFromYawPitchRoll(camYaw, 0.f, 0.f));
			Vec3 camForward = yawMat.Backward();
			Vec3 camRight   = yawMat.Right();

			// WASD 입력
			float ix = movementComponent->mMovingDirection.x;  // A/D  (-1 ~ 1)
			float iy = movementComponent->mMovingDirection.z;  // W/S   (-1 ~ 1)

			// 카메라 상대 방향 -> 월드 이동 방향
			Vec3 desired = camForward * iy + camRight * ix;
			const bool moving = desired.LengthSquared() > 0.0001f;
			if (moving)
				desired.Normalize();

			if (moving)
			{
				constexpr float kTurnSpeed = 12.f;
				const float targetYaw = DirectX::XMConvertToDegrees(atan2f(desired.x, desired.z));
				const float alpha = std::clamp(dt * kTurnSpeed, 0.f, 1.f);
				transformComponent->mLocalRotationE.y =
					LerpAngleDegrees(transformComponent->mLocalRotationE.y, targetYaw, alpha);
			}

			transformComponent->mLocalPosition += desired * dt * mainPlayerComponent->mSpeed;

			netTransformComponent->mStartPosition = transformComponent->mLocalPosition;

			netTransformComponent->mStartRotation.y = transformComponent->mLocalRotationE.y;
			netTransformComponent->mStartRotation.x = transformComponent->mLocalRotationE.x;

		}

		////jump
		//GravityComponent* gravityComponent = mWorld->GetComponent<GravityComponent>(entity);
		////cout << "height::" << gravityComponent->mHight << endl;
		//mainPlayerComponent->mFalling = gravityComponent->mFalling;
		//if (movementComponent->mJump) {
		//	gravityComponent->mHight += 10.0f;
		//	gravityComponent->mGravity -= mainPlayerComponent->mJumpPower;
		//	movementComponent->mJump = false;
		//	mainPlayerComponent->mFalling = true;
		//}
		
	}

	
	//bullet move
	for (size_t i = 0; i < mActiveBulletEntityIds.size();)
	{
		Entity bulletEntity{ mActiveBulletEntityIds[i] };
		BulletComponent* bulletComp = mWorld->GetComponent<BulletComponent>(bulletEntity);
		TransformComponent* bulletTransform = mWorld->GetComponent<TransformComponent>(bulletEntity);

		if (!bulletComp || !bulletTransform || !bulletComp->mIsActive)
		{
			UnregisterActiveBullet(bulletEntity);
			continue;
		}

		Vec3 direction = bulletComp->mDirection;
		if (direction.LengthSquared() <= 0.0001f)
			direction = Vec3::Forward;
		direction.Normalize();

		bulletTransform->mMovingVector = direction * bulletComp->mSpeed * dt;
		bulletTransform->mLocalPosition += bulletTransform->mMovingVector;
		bulletTransform->mWorldPosition = bulletTransform->mLocalPosition;

		bulletComp->UpdateLifeTime(dt);

		if (bulletComp->IsExpired())
		{
			bulletComp->Deactivate();
			bulletTransform->mMovingVector = Vec3::Zero;
			if (VfxComponent* bulletVfx = mWorld->GetComponent<VfxComponent>(bulletEntity))
			{
				// Fix: stop client-side bullet VFX when local lifetime prediction expires before the server deactivate packet arrives.
				bulletVfx->mShouldPlay = false;
				bulletVfx->mIsPaused = false;
				bulletVfx->mIsPlaying = false;
				bulletVfx->mScale = Vec3::Zero;
				bulletVfx->mTotalTime = 0.f;
			}
			UnregisterActiveBullet(bulletEntity);
			continue;
		}

		++i;
	}

}


void MovementSystem::RegisterActiveBullet(Entity bulletEntity)
{
	if (!bulletEntity.IsValid())
		return;

	const EntityID bulletId = bulletEntity.GetID();
	for (EntityID activeBulletId : mActiveBulletEntityIds)
	{
		if (activeBulletId == bulletId)
			return;
	}

	mActiveBulletEntityIds.push_back(bulletId);
}

void MovementSystem::UnregisterActiveBullet(Entity bulletEntity)
{
	if (!bulletEntity.IsValid())
		return;

	const EntityID bulletId = bulletEntity.GetID();
	for (size_t i = 0; i < mActiveBulletEntityIds.size(); ++i)
	{
		if (mActiveBulletEntityIds[i] != bulletId)
			continue;

		mActiveBulletEntityIds[i] = mActiveBulletEntityIds.back();
		mActiveBulletEntityIds.pop_back();
		return;
	}
}

void MovementSystem::ResetTurnInPlace(PlayerMovementComponent* movementComponent)
{
	if (!movementComponent)
		return;

	movementComponent->mIsTurnInPlace = false;
	movementComponent->mTurnInPlaceDir = 0;
	movementComponent->mTurnStartYaw = 0.f;
	movementComponent->mTurnTargetYaw = 0.f;
	movementComponent->mTurnElapsed = 0.f;
}



void MovementSystem::UpdateBodyYawForAim(PlayerMovementComponent* movementComponent, TransformComponent* transformComponent, float dt, bool followCameraImmediately)
{
	if (!movementComponent || !transformComponent)
		return;

	const float cameraYaw = movementComponent->mCameraRotationY;

	if (followCameraImmediately)
	{
		// 이동 중에는 기존 조작감을 유지하기 위해 몸 방향을 카메라 yaw에 즉시 맞춘다.
		transformComponent->mLocalRotationE.y = cameraYaw;
		ResetTurnInPlace(movementComponent);
		return;
	}

	constexpr float kTurnStartDeg = 75.f;

	const float deltaYaw = Wrap180Degrees(cameraYaw - transformComponent->mLocalRotationE.y);
	const float absDeltaYaw = fabsf(deltaYaw);

	if (!movementComponent->mIsTurnInPlace && absDeltaYaw > kTurnStartDeg)
	{
		// IP 시작 순간의 현재 몸 yaw와 최종 하체 yaw를 고정.
		// 이후에는 카메라를 계속 추적하지 않고 고정된 목표 yaw까지 블렌딩한다.
		movementComponent->mIsTurnInPlace = true;
		movementComponent->mTurnInPlaceDir = (deltaYaw >= 0.f) ? 1 : -1;
		movementComponent->mTurnStartYaw = transformComponent->mLocalRotationE.y;
		movementComponent->mTurnTargetYaw = cameraYaw;
		movementComponent->mTurnElapsed = 0.f;
	}

	if (!movementComponent->mIsTurnInPlace)
		return;

	movementComponent->mTurnElapsed += dt;

	const float duration = max(movementComponent->mTurnDuration, 0.0001f);
	const float alpha = std::clamp(movementComponent->mTurnElapsed / duration, 0.f, 1.f);
	const float blend = SmoothStep01(alpha);

	transformComponent->mLocalRotationE.y =
		LerpAngleDegrees(movementComponent->mTurnStartYaw, movementComponent->mTurnTargetYaw, blend);

	if (alpha >= 1.f)
	{
		// 블렌딩이 끝나면 시작 시점에 확정한 하체 목표 yaw로 마무리한다.
		// Turn 중 카메라가 더 움직였으면 다음 프레임의 yaw 차이 검사로 새 Turn이 다시 시작된다.
		transformComponent->mLocalRotationE.y = movementComponent->mTurnTargetYaw;
		ResetTurnInPlace(movementComponent);
	}
}
