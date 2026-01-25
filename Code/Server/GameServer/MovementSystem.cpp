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

MovementSystem::MovementSystem(World* world) : System(world)
{

}



void MovementSystem::Update(float dt) {

	//if (false == mWorld->HasComponentPool<MainCameraComponent>())return;
	//if (false == mWorld->HasComponentPool<PlayerMovementComponent>())return;

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
			//PlayerMovementComponent* movementComponent = mWorld->GetComponent<PlayerMovementComponent>(entity);
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

					mainPlayerComponent->mFsm.ChangeState(mainPlayerComponent, WalkState::Instance());
				}

				//dash
				//mainPlayerComponent->mFsm.ChangeState(mainPlayerComponent, DashState::Instance());

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

				//if (entity.GetID() != 1) {
				//	std::cout << "[MovementSystem] Player " << entity.GetID() << "Position: (" << transformComponent->mLocalPosition.x << ", " << transformComponent->mLocalPosition.y << ", " << transformComponent->mLocalPosition.z << ")";
				//	std::cout << "mx " << ix << "," << iy << std::endl;
				//	std::cout << "desired: (" << desired.x << ", " << desired.y << ", " << desired.z << ")" << std::endl;
				//	//std::cout << "dt: " << dt << ", speed: " << mainPlayerComponent->mSpeed << std::endl;
				//}
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
			//cout << "height::" << gravityComponent->mHight << endl;
			mainPlayerComponent->mFalling = gravityComponent->mFalling;
			if (inputComponent->MoveY == 1) {
				if (not mainPlayerComponent->mFalling) {
					gravityComponent->mHight += 10.0f;
					gravityComponent->mGravity -= mainPlayerComponent->mJumpPower;
					//stateSetting
					mainPlayerComponent->mFsm.ChangeState(mainPlayerComponent, JumpState::Instance());
				}
				mainPlayerComponent->mFalling = true;

			}


		}
	}
	//enemy movement
	/*std::vector<Entity> enemyEntitys{ mWorld->GetEntitiesWithComponent<EnemyMovementComponent>() };
	for (auto& entity : enemyEntitys) {
		TransformComponent* transformComponent = mWorld->GetComponent<TransformComponent>(entity);
		EnemyMovementComponent* enemyMovementComponent = mWorld->GetComponent<EnemyMovementComponent>(entity);

		transformComponent->mLocalPosition += enemyMovementComponent->mMovingDirection * dt * 50;
	}*/

}