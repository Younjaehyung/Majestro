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

MovementSystem::MovementSystem(World* world) : System(world)
{

}



void MovementSystem::Update(float dt) {
	if (false == mWorld->HasComponentPool<MainCameraComponent>())return;
	if (false == mWorld->HasComponentPool<PlayerMovementComponent>())return;

	//terrain
	auto terrainEntities = mWorld->GetEntitiesWithComponent<TerrainComponent>();
	TerrainComponent* terrainComponent = mWorld->GetComponent<TerrainComponent>(terrainEntities[0]);

	/*std::vector<Entity> gravityEntitys{ mWorld->GetEntitiesWithComponent<GravityComponent>() };
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

		NetTransformComponent * netTransformComponent = mWorld->GetComponent<NetTransformComponent>(entity);
		if (netTransformComponent == nullptr) {
			transformComponent->mLocalPosition.y = gravityComponent->mHight;
		}
		else {
			netTransformComponent->mStartPosition.y = transformComponent->mLocalPosition.y = gravityComponent->mHight;
		}
		
		
	}*/


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
			transformComponent->mLocalRotationE.y = movementComponent->mCameraRotationY;

			netTransformComponent->mStartPosition = transformComponent->mLocalPosition;
			netTransformComponent->mStartRotation.y = transformComponent->mLocalRotationE.y;
			netTransformComponent->mStartRotation.x = transformComponent->mLocalRotationE.x;

		
		}
		else if (cameraTypeComponent->mPlayMode == THREE_RPG) {
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

			if (mainPlayerComponent->mSpeed > 0) {
				transformComponent->mLocalRotationE.y = movementComponent->mCameraRotationY;
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

	////enemy movement
	//std::vector<Entity> enemyEntitys{ mWorld->GetEntitiesWithComponent<EnemyMovementComponent>() };
	//for (auto& entity : enemyEntitys) {
	//	TransformComponent* transformComponent = mWorld->GetComponent<TransformComponent>(entity);
	//	EnemyMovementComponent* enemyMovementComponent = mWorld->GetComponent<EnemyMovementComponent>(entity);
	//	
	//	transformComponent->mLocalPosition += enemyMovementComponent->mMovingDirection * dt * 50;
	//}

}


