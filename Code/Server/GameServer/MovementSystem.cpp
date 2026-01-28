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

	

	for (auto& cameraEntitys : mainCameraEntitys)
	{
		CameraTypeComponent* cameraTypeComponent = mWorld->GetComponent<CameraTypeComponent>(cameraEntitys);

		for (auto& entity : playerEntitys) {
			InputComponent* inputComponent = mWorld->GetComponent<InputComponent>(entity);

			TransformComponent* transformComponent = mWorld->GetComponent<TransformComponent>(entity);
			MainPlayerComponent* mainPlayerComponent = mWorld->GetComponent<MainPlayerComponent>(entity);

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

		if (!transformComponent || !enemyMovementComponent)
			continue;

		// ====== 기존 이동 로직 유지 ======
		transformComponent->mLocalPosition += enemyMovementComponent->mMovingDirection * dt * 50.0f;

		// ====== [추가] 이동 방향을 바라보도록 회전(yaw) 계산 + 천천히 회전 ======
		// 주의: mMovingDirection이 "월드 기준 이동 방향"이라고 가정.
		//       (로컬 기준이면 먼저 월드로 변환하거나, 반대로 로컬 회전과 정합 맞춰야 함.)
		Vec3 dir = enemyMovementComponent->mMovingDirection;

		// [추가] 방향 벡터가 거의 0이면 회전 계산하지 않음(각도 불안정 방지)
		const float lenSq = dir.x * dir.x + dir.y * dir.y + dir.z * dir.z;
		if (lenSq <= 1e-8f)
			continue;

		// [추가] 지면 이동이라면 y 성분 제거해서 수평 방향만으로 yaw 계산
		dir.y = 0.0f;
		const float flatLenSq = dir.x * dir.x + dir.z * dir.z;
		if (flatLenSq <= 1e-8f)
			continue;

		// [추가] 정규화(각도 계산 안정성)
		const float invLen = 1.0f / sqrtf(flatLenSq);
		dir.x *= invLen;
		dir.z *= invLen;

		// [추가] 목표 yaw 계산
		// - 엔진/모델의 전방(forward)이 +Z일 때: yaw = atan2(x, z)
		// - 만약 전방이 +X라면: atan2(z, x) 로 바꿔야 함
		const float targetYaw = atan2f(dir.x, dir.z) + 3.14159265358979323846f /*PI*/;

		// [추가] 현재 yaw -> 목표 yaw 로 "초당 kTurnSpeedRadPerSec"만큼만 회전
		const float maxDelta = kTurnSpeedRadPerSec * dt;
		transformComponent->mLocalRotation.y =
			MoveTowardsAngle(transformComponent->mLocalRotation.y, targetYaw, maxDelta);

		// (선택) pitch/roll 고정이 필요하면 아래 주석 해제
		// transformComponent->mLocalRotation.x = 0.0f;
		// transformComponent->mLocalRotation.z = 0.0f;
	}

	

}