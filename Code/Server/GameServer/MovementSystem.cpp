#include "pch.h"
#include "MovementSystem.h"
#include "TransformSystem.h"

#include "PhysicsWorld.h"

#include "TransformComponent.h"
#include "TerrainComponent.h"
#include "GravityComponent.h"
#include "MovementComponent.h"
#include "CameraComponent.h"
#include "TagComponent.h"
#include "PlayerComponent.h"
#include "InputComponent.h"
#include "BulletComponent.h"
#include "GameEvents.h"


static float WrapAngleDeg(float angleDeg)
{
	while (angleDeg > 180.0f) angleDeg -= 360.0f;
	while (angleDeg < -180.0f) angleDeg += 360.0f;
	return angleDeg;
}

// [추가] 현재 각도를 목표 각도로 "최대 변화량(maxDelta)"만큼만 따라가게 하는 함수(천천히 회전)
static float MoveTowardsAngleDeg(float currentDeg, float targetDeg, float maxDeltaDeg)
{
	float delta = WrapAngleDeg(targetDeg - currentDeg);

	if (delta > maxDeltaDeg) delta = maxDeltaDeg;
	if (delta < -maxDeltaDeg) delta = -maxDeltaDeg;

	return WrapAngleDeg(currentDeg + delta);
}


MovementSystem::MovementSystem(World* world) : System(world)
{
}

void MovementSystem::Update(float dt) {

	if (false == mWorld->HasComponentPool<MainCameraComponent>())return;
	if (false == mWorld->HasComponentPool<PlayerMovementComponent>())return;
	if (false == mWorld->HasComponentPool<EnemyMovementComponent>())return;

	//terrain
	//auto terrainEntities = mWorld->GetEntitiesWithComponent<TerrainComponent>();
	//TerrainComponent* terrainComponent = mWorld->GetComponent<TerrainComponent>(terrainEntities[0]);
	//


	auto terrainView = mWorld->View<TerrainComponent>();
	auto terrainIt = terrainView.begin();
	if (terrainIt == terrainView.end()) return;
	TerrainComponent* terrainComponent = mWorld->GetComponent<TerrainComponent>(*terrainIt);

	std::vector<Entity> gravityEntitys{ mWorld->GetEntitiesWithComponent<GravityComponent>() };
	for (auto& entity : gravityEntitys) {
		TransformComponent* transformComponent = mWorld->GetComponent<TransformComponent>(entity);
		GravityComponent* gravityComponent = mWorld->GetComponent<GravityComponent>(entity);
		float terrainGround = terrainComponent->GetHeightAtWorldPosition(transformComponent->mLocalPosition);
		float objectGround = mWorld->GetPhysicsWorld()->QueryHeightAtPosition(transformComponent->mLocalPosition);
		gravityComponent->mGround = max(terrainGround, objectGround);

		if (gravityComponent->mHight <= terrainGround || gravityComponent->mHight - gravityComponent->mHeightInterpolation <= terrainGround) {
			gravityComponent->mHight = terrainGround;
			gravityComponent->mGravity = 0.0f;

			gravityComponent->mFalling = false;
		}
		else {
			gravityComponent->mFalling = true;

			gravityComponent->mGravity += gravityComponent->mGravityA * dt;
			gravityComponent->mHight -= gravityComponent->mGravity * dt;
		}

		transformComponent->mLocalPosition.y = gravityComponent->mHight;

	}


	//main player movement
	std::vector<Entity> mainCameraEntitys{ mWorld->GetEntitiesWithComponent<MainCameraComponent>() };
	if (mainCameraEntitys.empty())return;
	std::vector<Entity> playerEntitys{ mWorld->GetEntitiesWithComponent<PlayerMovementComponent>() };

	//auto& playerMovePool = mWorld->GetComponentPool<PlayerMovementComponent>();

	for (auto& cameraEntity : mainCameraEntitys)
	{
		CameraTypeComponent* cameraTypeComponent = mWorld->GetComponent<CameraTypeComponent>(cameraEntity);

		for (auto& entity : playerEntitys) {
			if (entity.GetID() != cameraTypeComponent->mTargetID) continue;

			InputComponent* inputComponent = mWorld->GetComponent<InputComponent>(entity);

			TransformComponent* transformComponent = mWorld->GetComponent<TransformComponent>(entity);
			MainPlayerComponent* mainPlayerComponent = mWorld->GetComponent<MainPlayerComponent>(entity);

			//float correction{ 0 };
			//if (mainPlayerComponent->mPlayerType == 1 || mainPlayerComponent->mPlayerType == 2) correction = 3.14159265358979323846f;


			if (cameraTypeComponent->mPlayMode == ONE_FPS || cameraTypeComponent->mPlayMode == THREE_FPS) {
				Vec3 forward = transformComponent->GetLook();
				Vec3 right = transformComponent->GetRight();

				// WASD 입력
				float ix = inputComponent->MoveX;//movementComponent->mMovingDirection.x;  // A/D  (-1 ~ 1)
				float iy = inputComponent->MoveZ;//movementComponent->mMovingDirection.z;  // W/S   (-1 ~ 1)

				// 로컬 입력 방향을 월드 방향으로 변환
				Vec3 desired = forward * iy + right * ix;

				// 정규화
				if (desired.LengthSquared() > 0.0001f)
					desired.Normalize();

				transformComponent->mMovingVector = desired * dt * mainPlayerComponent->mSpeed;
				transformComponent->mLocalPosition += transformComponent->mMovingVector;

				transformComponent->mLocalRotationE.y = inputComponent->Yaw;//movementComponent->mCameraRotationY;

			}
			else if (cameraTypeComponent->mPlayMode == THREE_RPG) {
				Vec3 forward = transformComponent->GetLook();
				Vec3 right = transformComponent->GetRight();

				// WASD 입력
				float ix = inputComponent->MoveX;
				float iy = inputComponent->MoveZ;

				// 로컬 입력 방향을 월드 방향으로 변환
				Vec3 desired = forward * iy + right * ix;

				// 정규화
				if (desired.LengthSquared() > 0.0001f)
					desired.Normalize();

				if (mainPlayerComponent->mSpeed > 0) {
					transformComponent->mLocalRotationE.y = inputComponent->Yaw;
				}
				transformComponent->mLocalPosition += desired * dt * mainPlayerComponent->mSpeed;

			}

			//jump
			GravityComponent* gravityComponent = mWorld->GetComponent<GravityComponent>(entity);

			mainPlayerComponent->mFalling = gravityComponent->mFalling;
			if (inputComponent->IsButtonPressed(InputButtons::SPACE)) {
				if (not mainPlayerComponent->mFalling) {
					gravityComponent->mHight += 10.0f;
					gravityComponent->mGravity -= mainPlayerComponent->mJumpPower;
				}
				mainPlayerComponent->mFalling = true;

			}


		}
	}

	//enemy movement

	constexpr float kTurnSpeedDegPerSec = 360.0f;
	std::vector<Entity> enemyEntitys{ mWorld->GetEntitiesWithComponent<EnemyMovementComponent>() };
	for (auto& entity : enemyEntitys) {
		TransformComponent* transformComponent = mWorld->GetComponent<TransformComponent>(entity);
		EnemyMovementComponent* enemyMovementComponent = mWorld->GetComponent<EnemyMovementComponent>(entity);

		if (!transformComponent || !enemyMovementComponent) continue;

		transformComponent->mMovingVector = enemyMovementComponent->mMovingDirection * dt * enemyMovementComponent->mMovingSpeed;
		transformComponent->mLocalPosition += transformComponent->mMovingVector;

		Vec3 dir = enemyMovementComponent->mMovingDirection;

		const float lenSq = dir.x * dir.x + dir.y * dir.y + dir.z * dir.z;
		if (lenSq <= 1e-8f)
			continue;

		dir.y = 0.0f;
		const float flatLenSq = dir.x * dir.x + dir.z * dir.z;
		if (flatLenSq <= 1e-8f)
			continue;

		const float invLen = 1.0f / sqrtf(flatLenSq);
		dir.x *= invLen;
		dir.z *= invLen;

		constexpr float kRadToDeg = 57.295779513082320876f;
		const float targetYawDeg = atan2f(dir.x, dir.z) * kRadToDeg + 180.0f;

		const float maxDeltaDeg = kTurnSpeedDegPerSec * dt;
		transformComponent->mLocalRotationE.y =
			MoveTowardsAngleDeg(transformComponent->mLocalRotationE.y, targetYawDeg, maxDeltaDeg);

		// pitch/roll 고정이 필요하면 아래 주석 해제
		// transformComponent->mLocalRotation.x = 0.0f;
		// transformComponent->mLocalRotation.z = 0.0f;
	}

	//bullet move
	auto& activeBulletEntityIds = mWorld->GetActiveBulletEntityIds();
	for (size_t i = 0; i < activeBulletEntityIds.size();)
	{
		Entity entity{ activeBulletEntityIds[i] };
		BulletComponent* bulletComponent = mWorld->GetComponent<BulletComponent>(entity);
		TransformComponent* transformComponent = mWorld->GetComponent<TransformComponent>(entity);

		if (!bulletComponent || !transformComponent || !bulletComponent->mIsActive)
		{
			mWorld->UnregisterActiveBullet(entity);
			continue;
		}

		Vec3 direction = bulletComponent->mDirection;
		if (direction.LengthSquared() <= 0.0001f)
			direction = Vec3::Forward;
		direction.Normalize();

		transformComponent->mMovingVector = direction * bulletComponent->mSpeed * dt;
		transformComponent->mLocalPosition += transformComponent->mMovingVector;

		if (bulletComponent->UpdateLifeTime(dt))
		{
			bulletComponent->Deactivate();
			transformComponent->mMovingVector = Vec3::Zero;
			mWorld->UnregisterActiveBullet(entity);

			auto eventManager = mWorld->GetEventManager();
			if (eventManager)
			{
				eventManager->Enqueue<EvBulletDeactivated>(EvBulletDeactivated{ entity });
			}
			continue;
		}

		++i;
	}

}

