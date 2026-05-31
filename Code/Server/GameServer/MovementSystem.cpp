#include "pch.h"
#include "MovementSystem.h"
#include "TransformSystem.h"

#include "PhysicsWorld.h"

#include "TransformComponent.h"
#include "GravityComponent.h"
#include "MovementComponent.h"
#include "CameraComponent.h"
#include "TagComponent.h"
#include "PlayerComponent.h"
#include "EnemyComponent.h"
#include "InputComponent.h"
#include "BulletComponent.h"
#include "GameEvents.h"

float MovementSystem::WrapAngleDeg(float angleDeg)
{
	while (angleDeg > 180.0f) angleDeg -= 360.0f;
	while (angleDeg < -180.0f) angleDeg += 360.0f;
	return angleDeg;
}

// 현재 각도를 목표 각도로 "최대 변화량(maxDelta)"만큼만 따라가게 하는 함수(천천히 회전)
float MovementSystem::MoveTowardsAngleDeg(float currentDeg, float targetDeg, float maxDeltaDeg)
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

	UpdateEvent(dt);

	if (false == mWorld->HasComponentPool<MainCameraComponent>())return;
	if (false == mWorld->HasComponentPool<PlayerMovementComponent>())return;
	if (false == mWorld->HasComponentPool<EnemyMovementComponent>())return;


	UpdateGravity(dt);
	UpdatePlayer(dt);
	UpdateEnemy(dt);
	UpdateBullet(dt);

}

void MovementSystem::UpdateEvent(float dt)
{
	if (auto em = mWorld->GetEventManager())
	{
		em->Consume<EvImpulse>([&](const EvImpulse& e)
			{
				if (!e.target.IsValid()) return;

				if (auto* player = mWorld->GetComponent<MainPlayerComponent>(e.target))
				{
					const float invDt = dt > 0.0001f ? 1.0f / dt : 1.0f;
					player->mExternalVelocity = Vec3(e.x * invDt, e.y * invDt, e.z * invDt);
					player->mExternalMoveEndTime = GetServerTotalTimeSeconds() + 0.25f;

					switch (e.source)
					{
					case ImpulseSource::JumpPad:
						player->mExternalMoveMode = static_cast<uint8>(ReplicatedExternalMoveMode::OverrideY);
						break;

					case ImpulseSource::Knockback:
						player->mExternalMoveMode = static_cast<uint8>(ReplicatedExternalMoveMode::OverrideXZ);
						player->mCanControlHorizontal = false;
						break;

					case ImpulseSource::Wind:
					case ImpulseSource::Conveyor:
						player->mExternalMoveMode = static_cast<uint8>(ReplicatedExternalMoveMode::Additive);
						break;

					case ImpulseSource::Script:
						player->mExternalMoveMode = static_cast<uint8>(ReplicatedExternalMoveMode::OverrideAll);
						break;
					}
				}

				if (auto* tr = mWorld->GetComponent<TransformComponent>(e.target))
				{
					tr->mLocalPosition.x += e.x;
					tr->mLocalPosition.z += e.z;

					tr->mMovingVector.x += e.x;
					tr->mMovingVector.z += e.z;
				}

				if (e.y != 0.0f)
				{
					if (auto* gr = mWorld->GetComponent<GravityComponent>(e.target))
					{

						gr->mGravity -= e.y; // gravity가 양수면 하강 속도. 위로 쏘려면 음수 방향
						gr->mHight += 20.0f; // 지면에 붙어 있을 때 즉시 재착지로 상쇄되는 것을 막기 위해 띄움 
						gr->mFalling = true;
					}
				}
			});
	}

}

void MovementSystem::UpdatePlayer(float dt)
{
	//main player movement
	std::vector<Entity> mainCameraEntitys{ mWorld->GetEntitiesWithComponent<MainCameraComponent>() };
	if (mainCameraEntitys.empty())return;
	std::vector<Entity> playerEntitys{ mWorld->GetEntitiesWithComponent<PlayerMovementComponent>() };

	for (auto& cameraEntity : mainCameraEntitys)
	{
		CameraTypeComponent* cameraTypeComponent = mWorld->GetComponent<CameraTypeComponent>(cameraEntity);

		for (auto& entity : playerEntitys) {
			if (entity.GetID() != cameraTypeComponent->mTargetID.GetID()) continue;

			InputComponent* inputComponent = mWorld->GetComponent<InputComponent>(entity);

			TransformComponent* transformComponent = mWorld->GetComponent<TransformComponent>(entity);
			MainPlayerComponent* mainPlayerComponent = mWorld->GetComponent<MainPlayerComponent>(entity);

			//jump
			GravityComponent* gravityComponent = mWorld->GetComponent<GravityComponent>(entity);

			mainPlayerComponent->mFalling = gravityComponent->mFalling;
			if (inputComponent->IsButtonPressed(InputButtons::SPACE) && mainPlayerComponent->CanUseVerticalInput()) {
				if (not mainPlayerComponent->mFalling) {
					gravityComponent->mHight += 20.0f;
					gravityComponent->mGravity -= mainPlayerComponent->mJumpPower;
				}
				mainPlayerComponent->mFalling = true;

			}

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

			TransformComponent* tf = mWorld->GetComponent<TransformComponent>(entity);
			if (!tf) continue;
			static constexpr float MIN_MOVE_SQ = 0.01f * 0.01f; // cm 단위
			shared_ptr<Navigation>& nav = mWorld->GetNavSystem();
			if (!nav || !nav->IsInitialized()) return;

			if (gravityComponent && gravityComponent->mDropping)
				continue;

			// XZ 이동량이 거의 없으면 검증 불필요
			const float moveXSq = tf->mMovingVector.x * tf->mMovingVector.x
				+ tf->mMovingVector.z * tf->mMovingVector.z;
			if (moveXSq < MIN_MOVE_SQ) continue;

			// 이동 전 XZ 위치 복원 (mMovingVector = 이번 프레임 이동 델타)
			Vec3 prevPos = tf->mLocalPosition;
			prevPos.x -= tf->mMovingVector.x;
			prevPos.z -= tf->mMovingVector.z;
			// Y: 이동 전후 동일 (Y 검증은 중력 시스템에 위임)
			prevPos.y = tf->mLocalPosition.y;

			// 입력으로 이미 옮겨진 목표 XZ 
			const Vec3 desiredEnd = tf->mLocalPosition;

			// NavMesh 표면을 따라 이동 — 벽/경계가 있으면 자동으로 막히는 위치로 클램프
			Vec3 resultPos;
			if (nav->MoveAlongSurface(prevPos, desiredEnd, resultPos))
			{
				// 경계에 막혀 클램프됐는지 판정 
				static constexpr float kEdgeBlockEpsSq = 2.0f * 2.0f; // (2cm)^2
				const float clampDx = desiredEnd.x - resultPos.x;
				const float clampDz = desiredEnd.z - resultPos.z;
				const bool blocked = (clampDx * clampDx + clampDz * clampDz) > kEdgeBlockEpsSq;

				bool dropped = false;
				if (blocked && gravityComponent)
				{
					// 막힌 경계가 떨어질 수 있는 단차인지 Jolt로 판별 후 가능하면 낙하 시작
					dropped = TryStartEdgeDrop(entity, tf, gravityComponent, prevPos, desiredEnd);
				}

				if (!dropped && gravityComponent)
				{
					// 일반 클램프: 벽 또는 드롭 불가 경계
					tf->mLocalPosition.x = resultPos.x;
					//tf->mLocalPosition.y = resultPos.y; // Y는 NavMesh 높이로 보정 (낙하/점프는 중력 시스템에 위임)
					tf->mLocalPosition.z = resultPos.z;
					tf->mMovingVector.x = resultPos.x - prevPos.x;
					tf->mMovingVector.y = 0;
					tf->mMovingVector.z = resultPos.z - prevPos.z;

					gravityComponent->mGround = resultPos.y;
					//gravityComponent->mHight = resultPos.y; // NavMesh 높이는 중력 단계에서 적용되도록 저장만 수행
				}
				// dropped == true: XZ는 desiredEnd 유지, 낙하는 TryStartEdgeDrop에서 시작됨
			}
			// MoveAlongSurface가 false(NavMesh 밖)이면 검증 스킵  이동 그대로


		}
	}


}

bool MovementSystem::TryStartEdgeDrop(Entity entity, TransformComponent* tf, GravityComponent* grav,
	const Vec3& prevPos, const Vec3& desiredEnd)
{
	static constexpr float kMaxDropHeight   = 800.0f; // 허용 최대 낙차
	static constexpr float kDropProbeStepUp = 10.0f;  // 발 높이 위쪽은 거의 안 봄
	static constexpr float kWallProbeUp     = 60.0f;  // 벽 검사 캐스트 높이(가슴)
	static constexpr float kWallProbeRadius = 20.0f;  // 벽 검사 구 반경

	auto physics = mWorld->GetPhysicsWorld();
	if (!physics)
		return false;

	// 발 밑에 XZ 아래에 착지 가능한 바닥이 있는지
	float landY = 0.0f;
	if (!physics->TryQueryTerrainHeightNear(desiredEnd, tf->mLocalPosition.y,
		kDropProbeStepUp, kMaxDropHeight, landY))
		return false; // 아래에 바닥 없음(허공)

	const float drop = tf->mLocalPosition.y - landY;
	if (drop <= grav->mStepDownDistance) // 계단 수준 단차는 기존 step-down 로직으로
		return false;
	if (drop > kMaxDropHeight)            // 너무 높음
		return false;

	//  벽 관통 방지: 가슴 높이에서 엣지 너머로 물리 벽이 막는지 검사
	Vec3 wallStart = prevPos;    
	wallStart.y += kWallProbeUp;
	Vec3 wallEnd   = desiredEnd; 
	wallEnd.y   += kWallProbeUp;
	//    (발 높이 캐스트는 서 있는 바닥에 오탐하므로 반드시 올려서 캐스트)

	JoltStaticHit hit;
	if (physics->CastMovingSphereAgainstStatic(wallStart, wallEnd, kWallProbeRadius, hit) && hit.hit)	// 벽이 막고 있음 
		return false; // 드롭 불가

	// 드롭 허용 (낙하 시작)
	grav->mGround   = landY;
	grav->mDropping = true;
	grav->mFalling  = true;
	return true;
}

void MovementSystem::UpdateEnemy(float dt)
{
	//enemy movement
	constexpr float kTurnSpeedDegPerSec = 360.0f;
	static constexpr float MIN_MOVE_SQ = 0.01f * 0.01f;
	shared_ptr<Navigation>& nav = mWorld->GetNavSystem();
	std::vector<Entity> enemyEntitys{ mWorld->GetEntitiesWithComponent<EnemyMovementComponent>() };
	for (auto& entity : enemyEntitys) {
		TransformComponent* transformComponent = mWorld->GetComponent<TransformComponent>(entity);
		EnemyMovementComponent* enemyMovementComponent = mWorld->GetComponent<EnemyMovementComponent>(entity);

		if (!transformComponent || !enemyMovementComponent) continue;

		transformComponent->mMovingVector = enemyMovementComponent->mMovingDirection * dt * enemyMovementComponent->mMovingSpeed;
		transformComponent->mLocalPosition += transformComponent->mMovingVector;

		if (nav && nav->IsInitialized())
		{
			const float moveXSq = transformComponent->mMovingVector.x * transformComponent->mMovingVector.x
				+ transformComponent->mMovingVector.z * transformComponent->mMovingVector.z;

			if (moveXSq >= MIN_MOVE_SQ)
			{
				Vec3 prevPos = transformComponent->mLocalPosition;
				prevPos.x -= transformComponent->mMovingVector.x;
				prevPos.z -= transformComponent->mMovingVector.z;
				prevPos.y = transformComponent->mLocalPosition.y;

				Vec3 resultPos;
				if (nav->MoveAlongSurface(prevPos, transformComponent->mLocalPosition, resultPos))
				{
					transformComponent->mLocalPosition.x = resultPos.x;
					transformComponent->mLocalPosition.z = resultPos.z;
					transformComponent->mMovingVector.x = resultPos.x - prevPos.x;
					transformComponent->mMovingVector.y = 0.0f;
					transformComponent->mMovingVector.z = resultPos.z - prevPos.z;

					GravityComponent* gravityComp = mWorld->GetComponent<GravityComponent>(entity);
					if (gravityComp)
						gravityComp->mGround = resultPos.y;
				}
			}
		}

		Vec3 dir = enemyMovementComponent->mMovingDirection;

		const float lenSq = dir.x * dir.x + dir.y * dir.y + dir.z * dir.z;
		if (lenSq <= 1e-8f)
		{
			EnemyComponent* enemyComp = mWorld->GetComponent<EnemyComponent>(entity);
			if (!enemyComp) continue;

			const Vec3 myPos = transformComponent->mLocalPosition;
			float nearestPlayerDistSq = (std::numeric_limits<float>::max)();
			Vec3 lookDir = Vec3::Zero;

			for (auto& playerEntity : mWorld->GetEntitiesWithComponent<PlayerMovementComponent>())
			{
				TransformComponent* playerTf = mWorld->GetComponent<TransformComponent>(playerEntity);
				if (!playerTf) continue;

				Vec3 toPlayer = playerTf->mLocalPosition - myPos;
				toPlayer.y = 0.f;
				const float distSq = toPlayer.LengthSquared();
				if (distSq < nearestPlayerDistSq)
				{
					nearestPlayerDistSq = distSq;
					lookDir = toPlayer;
				}
			}

			// 공격 사거리 내에서는 이동하지 않아도 타겟을 바라보도록 회전만 갱신
			if (nearestPlayerDistSq > enemyComp->AttackRangeSq || lookDir.LengthSquared() <= 1e-8f)
				continue;

			dir = lookDir;
		}

		dir.y = 0.0f;
		const float flatLenSq = dir.x * dir.x + dir.z * dir.z;
		if (flatLenSq <= 1e-8f)
			continue;

		const float invLen = 1.0f / sqrtf(flatLenSq);
		dir.x *= invLen;
		dir.z *= invLen;

		constexpr float kRadToDeg = 57.295779513082320876f;
		const float targetYawDeg = atan2f(dir.x, dir.z) * kRadToDeg;

		const float maxDeltaDeg = kTurnSpeedDegPerSec * dt;
		transformComponent->mLocalRotationE.y =
			MoveTowardsAngleDeg(transformComponent->mLocalRotationE.y, targetYawDeg, maxDeltaDeg);

		// pitch/roll 고정이 필요하면 아래 주석 해제
		// transformComponent->mLocalRotation.x = 0.0f;
		// transformComponent->mLocalRotation.z = 0.0f;
	}


}

void MovementSystem::UpdateGravity(float dt)
{
	std::vector<Entity> gravityEntitys{ mWorld->GetEntitiesWithComponent<GravityComponent>() };
	for (auto& entity : gravityEntitys) {
		TransformComponent* transformComponent = mWorld->GetComponent<TransformComponent>(entity);
		GravityComponent* gravityComponent = mWorld->GetComponent<GravityComponent>(entity);
		if (!transformComponent || !gravityComponent)
			continue;

	
		float baseGround = gravityComponent->mGround;
		float meshGround = 0.0f;
		auto physicsWorld = mWorld->GetPhysicsWorld();
		static constexpr float kGroundProbeStepUp = 120.0f;
		static constexpr float kGroundProbeDropDown = 500.0f;


		// 낙하 중이면 발 밑 탐색
		const bool airborne = gravityComponent->mFalling || gravityComponent->mDropping;
		const float probeAnchor   = airborne ? gravityComponent->mHight : baseGround;
		const float probeStepUp   = airborne ? 10.0f   : kGroundProbeStepUp;
		const float probeDropDown = airborne ? 5000.0f : kGroundProbeDropDown;

		if (physicsWorld)
		{
			if (physicsWorld->TryQueryTerrainHeightNear(
				transformComponent->mLocalPosition,
				probeAnchor, probeStepUp, probeDropDown, meshGround))
			{
				gravityComponent->mGround = meshGround;
			}
			else
			{	// 발밑에 물리 바닥이 전혀 없음
				// 낙하 시작/유지
				gravityComponent->mGround = -100000.0f;		// 착지 지점 X
				gravityComponent->mFalling = true;
			}
		}
		else
		{
			gravityComponent->mGround = baseGround;
		}

		if (gravityComponent->mHight <= gravityComponent->mGround || gravityComponent->mHight - gravityComponent->mHeightInterpolation <= gravityComponent->mGround) {
			gravityComponent->mHight = gravityComponent->mGround;
			gravityComponent->mGravity = 0.0f;

			gravityComponent->mFalling = false;
			gravityComponent->mDropping = false; // NavMesh 클램프 재개
		}
		else {
			// Step-down : 계단을 내려가는 케이스 ( falling 막기 )
			const float horizontalMoveSq =
				transformComponent->mMovingVector.x * transformComponent->mMovingVector.x
				+ transformComponent->mMovingVector.z * transformComponent->mMovingVector.z;
			const bool hasHorizontalMove = horizontalMoveSq > 0.0001f;
			const float distToGround = gravityComponent->mHight - gravityComponent->mGround;

			if (gravityComponent->mGravity >= 0.0f
				&& hasHorizontalMove
				&& distToGround <= gravityComponent->mStepDownDistance)
			{
				gravityComponent->mHight = gravityComponent->mGround;
				gravityComponent->mGravity = 0.0f;
				gravityComponent->mFalling = false;
				gravityComponent->mDropping = false;
			}
			else {
				gravityComponent->mFalling = true;

				gravityComponent->mGravity += gravityComponent->mGravityA * dt;
				gravityComponent->mHight -= gravityComponent->mGravity * dt;
			}
		}

		transformComponent->mLocalPosition.y = gravityComponent->mHight + 3.f;

	}
}

void MovementSystem::UpdateBullet(float dt)
{
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
		bulletComponent->UpdateLifeTime(dt);

		if (bulletComponent->IsExpired())
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

