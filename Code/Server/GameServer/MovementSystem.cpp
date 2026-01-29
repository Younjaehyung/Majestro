#include "pch.h"
#include "MovementSystem.h"
#include "TransformSystem.h"
#include "TransformComponent.h"
#include "TerrainComponent.h"
#include "GravityComponent.h"
#include "MovementComponent.h"
#include "CameraComponent.h"
#include "TagComponent.h"
#include "PlayerComponent.h"
#include "InputComponent.h"

static float WrapPi(float a)
{
	constexpr float PI = 3.14159265358979323846f;
	constexpr float TAU = 6.28318530717958647692f;

	while (a > PI) a -= TAU;
	while (a < -PI) a += TAU;
	return a;
}

// [추가] 현재 각도를 목표 각도로 "최대 변화량(maxDelta)"만큼만 따라가게 하는 함수(천천히 회전)
static float MoveTowardsAngle(float current, float target, float maxDelta)
{
	float delta = WrapPi(target - current);

	if (delta > maxDelta) delta = maxDelta;
	if (delta < -maxDelta) delta = -maxDelta;

	return WrapPi(current + delta);
}


MovementSystem::MovementSystem(World* world) : System(world)
{
}

void MovementSystem::Update(float dt) {

	if (false == mWorld->HasComponentPool<MainCameraComponent>())return;
	if (false == mWorld->HasComponentPool<PlayerMovementComponent>())return;
	if (false == mWorld->HasComponentPool<EnemyMovementComponent>())return;

	//terrain
	auto terrainEntities = mWorld->GetEntitiesWithComponent<TerrainComponent>();
	TerrainComponent* terrainComponent = mWorld->GetComponent<TerrainComponent>(terrainEntities[0]);
	

	std::vector<Entity> gravityEntitys{ mWorld->GetEntitiesWithComponent<GravityComponent>() };
	for (auto& entity : gravityEntitys) {
		TransformComponent* transformComponent = mWorld->GetComponent<TransformComponent>(entity);
		GravityComponent* gravityComponent = mWorld->GetComponent<GravityComponent>(entity);
		float terrainGround = terrainComponent->GetHeightAtWorldPosition(transformComponent->mLocalPosition);
		gravityComponent->mGround = terrainGround;

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

	auto& playerMovePool = mWorld->GetComponentPool<PlayerMovementComponent>();

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

			//stateSetting
			{
				if (inputComponent->MoveX == 0 && inputComponent->MoveZ==0) {
					mainPlayerComponent->mSpeed = 0.f;
					mainPlayerComponent->mFsm.ChangeState(mainPlayerComponent, IdleState::Instance());
				}
				else {
					if (mainPlayerComponent->GetState() & S_Dash)mainPlayerComponent->mSpeed = mainPlayerComponent->mDashSpeed;
					else mainPlayerComponent->mSpeed = mainPlayerComponent->mRunSpeed;

					
				}
			}

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

				transformComponent->mLocalPosition += desired * dt * mainPlayerComponent->mSpeed;

				transformComponent->mLocalRotation.y = inputComponent->Yaw;//movementComponent->mCameraRotationY;

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
					transformComponent->mLocalRotation.y = inputComponent->Yaw;
				}
				transformComponent->mLocalPosition += desired * dt * mainPlayerComponent->mSpeed;

			}

			//jump
			GravityComponent* gravityComponent = mWorld->GetComponent<GravityComponent>(entity);

			mainPlayerComponent->mFalling = gravityComponent->mFalling;
			if (inputComponent->MoveY == 1/*inputComponent->IsButtonPressed(InputButtons::SPACE)*/) {
				cout << "jump" << endl;
				if (not mainPlayerComponent->mFalling) {
					gravityComponent->mHight += 10.0f;
					gravityComponent->mGravity -= mainPlayerComponent->mJumpPower;
					//stateSetting
				}
				mainPlayerComponent->mFalling = true;

			}


		}
	}

	//enemy movement

	constexpr float kTurnSpeedRadPerSec = 6.0f;
	std::vector<Entity> enemyEntitys{ mWorld->GetEntitiesWithComponent<EnemyMovementComponent>() };
	for (auto& entity : enemyEntitys) {
		TransformComponent* transformComponent = mWorld->GetComponent<TransformComponent>(entity);
		EnemyMovementComponent* enemyMovementComponent = mWorld->GetComponent<EnemyMovementComponent>(entity);

		if (!transformComponent || !enemyMovementComponent) continue;

		
		transformComponent->mLocalPosition += enemyMovementComponent->mMovingDirection * dt * 50.0f;

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

		const float targetYaw = atan2f(dir.x, dir.z) + 3.14159265358979323846f /*PI*/;

		const float maxDelta = kTurnSpeedRadPerSec * dt;
		transformComponent->mLocalRotation.y =
			MoveTowardsAngle(transformComponent->mLocalRotation.y, targetYaw, maxDelta);

		// pitch/roll 고정이 필요하면 아래 주석 해제
		// transformComponent->mLocalRotation.x = 0.0f;
		// transformComponent->mLocalRotation.z = 0.0f;
	}

	

}