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

MovementSystem::MovementSystem(World* world) : System(world)
{

}



void MovementSystem::Update(float dt) {
	if (false == mWorld->HasComponentPool<MainCameraComponent>())return;
	if (false == mWorld->HasComponentPool<PlayerMovementComponent>())return;

	//terrain
	auto terrainEntities = mWorld->GetEntitiesWithComponent<TerrainComponent>();
	TerrainComponent* terrainComponent = mWorld->GetComponent<TerrainComponent>(terrainEntities[0]);


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

			//transformComponent->mLocalRotation.y = movementComponent->mCameraRotationY;
			if (!isLobbyScene || hasMoveInput) {
				transformComponent->mLocalRotationE.y = movementComponent->mCameraRotationY;
			}
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

		if (bulletComp->UpdateLifeTime(dt))
		{
			bulletComp->Deactivate();
			bulletTransform->mMovingVector = Vec3::Zero;
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