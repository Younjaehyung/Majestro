#include "pch.h"
#include "MovementSystem.h"
#include "TransformSystem.h"

#include "PhysicsWorld.h"

#include "TransformComponent.h"
#include "GravityComponent.h"
#include "FlyComponent.h"
#include "MovementComponent.h"
#include "CameraComponent.h"
#include "TagComponent.h"
#include "PlayerComponent.h"
#include "EnemyComponent.h"
#include "InputComponent.h"
#include "BulletComponent.h"
#include "ColliderComponent.h"
#include "GameEvents.h"

namespace
{
	float GetColliderRadiusXZ(const BoxColliderComponent* collider)
	{
		if (!collider)
			return 0.0f;

		const Vec3 half = collider->mHalfExtents;
		return std::sqrt(half.x * half.x + half.z * half.z);
	}

	Vec3 ResolvePlayerMoveAgainstEnemies(World* world, Entity playerEntity, const Vec3& startPos, const Vec3& desiredEnd)
	{
		if (!world || !playerEntity.IsValid())
			return desiredEnd;

		const float moveX = desiredEnd.x - startPos.x;
		const float moveZ = desiredEnd.z - startPos.z;
		const float moveLenSq = moveX * moveX + moveZ * moveZ;
		if (moveLenSq <= 1e-8f)
			return desiredEnd;

		static constexpr float kPlayerRadius = 30.0f;
		static constexpr float kEnemyBlockSkin = 2.0f;

		float earliestT = 1.0f;
		bool blocked = false;

		for (const Entity& enemyEntity : world->GetEntitiesWithComponents<EnemyMovementComponent, TransformComponent, BoxColliderComponent>())
		{
			if (!enemyEntity.IsValid() || enemyEntity == playerEntity)
				continue;

			const TransformComponent* enemyTransform = world->GetComponent<TransformComponent>(enemyEntity);
			const BoxColliderComponent* enemyCollider = world->GetComponent<BoxColliderComponent>(enemyEntity);
			const EnemyComponent* enemyComp = world->GetComponent<EnemyComponent>(enemyEntity);
			if (!enemyTransform || !enemyCollider || !enemyComp)
				continue;

			if (enemyComp->mAnimState == static_cast<uint8>(EnemyAnimState::Dead))
				continue;

			const float combinedRadius = kPlayerRadius + GetColliderRadiusXZ(enemyCollider);
			if (combinedRadius <= 0.0f)
				continue;

			const float cx = enemyTransform->mLocalPosition.x;
			const float cz = enemyTransform->mLocalPosition.z;
			const float sx = startPos.x - cx;
			const float sz = startPos.z - cz;

			if (sx * sx + sz * sz <= combinedRadius * combinedRadius)
				continue;

			const float a = moveLenSq;
			const float b = 2.0f * (sx * moveX + sz * moveZ);
			const float c = sx * sx + sz * sz - combinedRadius * combinedRadius;
			const float discriminant = b * b - 4.0f * a * c;
			if (discriminant < 0.0f)
				continue;

			const float sqrtDiscriminant = std::sqrt(discriminant);
			const float invDenom = 0.5f / a;
			const float t0 = (-b - sqrtDiscriminant) * invDenom;
			const float t1 = (-b + sqrtDiscriminant) * invDenom;
			const float hitT = (t0 >= 0.0f && t0 <= 1.0f) ? t0 : ((t1 >= 0.0f && t1 <= 1.0f) ? t1 : -1.0f);
			if (hitT < 0.0f)
				continue;

			if (hitT < earliestT)
			{
				earliestT = hitT;
				blocked = true;
			}
		}

		if (!blocked)
			return desiredEnd;

		const float moveLen = std::sqrt(moveLenSq);
		const float safeDistance = (std::max)(0.0f, moveLen * earliestT - kEnemyBlockSkin);
		const float invMoveLen = moveLen > 1e-6f ? 1.0f / moveLen : 0.0f;

		Vec3 resolved = startPos;
		resolved.x += moveX * invMoveLen * safeDistance;
		resolved.z += moveZ * invMoveLen * safeDistance;
		return resolved;
	}
}

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
	UpdateFlyHeight(dt);
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
					GravityComponent* gr = mWorld->GetComponent<GravityComponent>(e.target);
					Vec3 prevPos = tr->mLocalPosition;
					Vec3 desiredEnd = prevPos;
					desiredEnd.x += e.x;
					desiredEnd.z += e.z;

					Vec3 resolved = ResolvePlayerMoveByJolt(e.target, gr, prevPos, desiredEnd);
					tr->mLocalPosition.x = resolved.x;
					tr->mLocalPosition.z = resolved.z;
					tr->mMovingVector.x += resolved.x - prevPos.x;
					tr->mMovingVector.z += resolved.z - prevPos.z;
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
					// 점프 = 공중. UpdateGravity 가 다음 프레임부터 공중 낙하하도록 표시.
					gravityComponent->mFalling = true;
					gravityComponent->mDropping = false;
				}
				mainPlayerComponent->mFalling = true;

			}

			//float correction{ 0 };
			//if (mainPlayerComponent->mPlayerType == 1 || mainPlayerComponent->mPlayerType == 2) correction = 3.14159265358979323846f;


			Vec3 desiredMove = Vec3::Zero;
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

				desiredMove = desired * dt * mainPlayerComponent->mSpeed;

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
			
				desiredMove = desired * dt * mainPlayerComponent->mSpeed;

			}

			
			transformComponent->mMovingVector = desiredMove;
			transformComponent->mLocalPosition += transformComponent->mMovingVector;

			TransformComponent* tf = transformComponent;
			if (!tf) continue;
			static constexpr float MIN_MOVE_SQ = 0.01f * 0.01f; // cm 단위

			// 이동 전 XZ 위치 복원 (mMovingVector = 이번 프레임 입력 이동 델타)
			Vec3 prevPos = tf->mLocalPosition;
			prevPos.x -= tf->mMovingVector.x;
			prevPos.z -= tf->mMovingVector.z;
			prevPos.y = tf->mLocalPosition.y; // Y 는 동일(낙하/지면은 UpdateGravity가 함)

			// 입력으로 이미 옮겨진 목표 XZ
			const Vec3 desiredEnd = tf->mLocalPosition;

			// XZ 이동량이 거의 없으면 수평 처리 스킵 (Y 는 UpdateGravity 가 계속 처리)
			const float moveXSq = tf->mMovingVector.x * tf->mMovingVector.x
				+ tf->mMovingVector.z * tf->mMovingVector.z;
			if (moveXSq < MIN_MOVE_SQ)
			{
				tf->mMovingVector.x = 0.0f;
				tf->mMovingVector.z = 0.0f;
			}

			//  벽 처리 : Jolt StaticCollision 구 sweep + 슬라이드
			Vec3 resolved = ResolvePlayerMoveByJolt(entity, gravityComponent, prevPos, desiredEnd);

			// 이동 가능 영역 가드: 지면에 있을 때만 NavMesh 밖이면 차단 
			const bool airborne = gravityComponent
				&& (gravityComponent->mFalling || gravityComponent->mDropping);
			shared_ptr<Navigation>& nav = mWorld->GetNavSystem();
			if (!airborne && nav && nav->IsInitialized())
			{
				static constexpr float kNavGuardRadius = 120.0f;
				if (!nav->IsPointOnNavMesh(resolved, kNavGuardRadius))
				{
					resolved.x = prevPos.x;
					resolved.z = prevPos.z;
				}
			}

			tf->mLocalPosition.x = resolved.x;
			tf->mLocalPosition.z = resolved.z;
			tf->mMovingVector.x = resolved.x - prevPos.x;
			tf->mMovingVector.y = 0.0f;
			tf->mMovingVector.z = resolved.z - prevPos.z;
			// Y/지면/단차 낙하는 전부 UpdateGravity 가 Jolt 하향 레이로 처리
		}
	}


}

Vec3 MovementSystem::ResolvePlayerMoveByJolt(Entity playerEntity, GravityComponent* grav, const Vec3& prevPos, const Vec3& desiredEnd)
{
	Vec3 totalMove = desiredEnd - prevPos;
	totalMove.y = 0.0f;

	const float moveLen = sqrtf(totalMove.x * totalMove.x + totalMove.z * totalMove.z);
	static constexpr float kMaxStepDistance = 20.0f;
	const int subStepCount = (std::max)(1, (std::min)(12, static_cast<int>(ceilf(moveLen / kMaxStepDistance))));

	Vec3 resolved = prevPos;
	for (int step = 0; step < subStepCount; ++step)
	{
		Vec3 stepEnd = resolved;
		stepEnd.x += totalMove.x / subStepCount;
		stepEnd.z += totalMove.z / subStepCount;
		const Vec3 staticResolved = SweepSlideHorizontal(grav, resolved, stepEnd);
		resolved = ResolvePlayerMoveAgainstEnemies(mWorld, playerEntity, resolved, staticResolved);
	}

	return resolved;
}

Vec3 MovementSystem::SweepSlideHorizontal(GravityComponent* grav, const Vec3& prevPos, const Vec3& desiredEnd)
{
	Vec3 result = prevPos; // Y 는 prevPos.y 유지

	auto physics = mWorld->GetPhysicsWorld();
	Vec3 remaining = desiredEnd - prevPos;
	remaining.y = 0.0f;

	if (!physics)
	{
		// 입력대로 이동
		result.x = desiredEnd.x;
		result.z = desiredEnd.z;
		return result;
	}

	static constexpr float kPlayerRadius = 30.0f;
	static constexpr float kSkin         = 2.0f;
	static constexpr int   kMaxDepenIters = 3;
	static constexpr float kMaxDepenStep = 12.0f;
	static constexpr int   kMaxIters     = 5;     // 슬라이드 반복

	// 벽 sweep 구의 바닥을 발 위 stepClear 만큼 띄워서
	// 오를 수 있는 단은 구 아래로 통과시켜 벽으로 오인하지 않고 그보다 높은 연속 벽만 막는다.
	const float stepClear = grav ? grav->mStepDownDistance : 60.0f;
	const float feetY     = grav ? grav->mHight : prevPos.y;
	const float castY     = feetY + stepClear + kPlayerRadius;

	auto depenetrate = [&]()
	{
		for (int i = 0; i < kMaxDepenIters; ++i)
		{
			Vec3 correction = Vec3::Zero;
			const Vec3 probe(result.x, castY, result.z);
			if (!physics->ResolveSphereOverlapAgainstStatic(probe, kPlayerRadius, correction))
				break;

			correction.y = 0.0f;
			float corrLen = sqrtf(correction.x * correction.x + correction.z * correction.z);
			if (corrLen <= 1e-3f)
				break;

			if (corrLen > kMaxDepenStep)
			{
				const float scale = kMaxDepenStep / corrLen;
				correction.x *= scale;
				correction.z *= scale;
				corrLen = kMaxDepenStep;
			}

			
			result.x += correction.x + (correction.x / corrLen) * kSkin;
			result.z += correction.z + (correction.z / corrLen) * kSkin;
		}
	};

	depenetrate();

	for (int iter = 0; iter < kMaxIters; ++iter)
	{
		depenetrate();

		const float len = sqrtf(remaining.x * remaining.x + remaining.z * remaining.z);
		if (len <= 1e-3f)
			break;

		const Vec3 dir(remaining.x / len, 0.0f, remaining.z / len);
		const Vec3 castStart(result.x, castY, result.z);
		const Vec3 castEnd(result.x + remaining.x, castY, result.z + remaining.z);

		JoltStaticHit hit;
		if (physics->CastMovingSphereAgainstStatic(castStart, castEnd, kPlayerRadius, hit) && hit.hit)
		{
			// 벽 직전까지 전진
			const float safe = (std::max)(0.0f, hit.distance - kSkin);
			result.x += dir.x * safe;
			result.z += dir.z * safe;

			// 남은 이동을 벽 평면으로 투영해서 벽을 따라 슬라이드
			Vec3 n = hit.normal; n.y = 0.0f;
			const float nLen = sqrtf(n.x * n.x + n.z * n.z);
			if (nLen <= 1e-4f)
				break; // 법선 불명(수평 성분 없으므로 정지
			n.x /= nLen; n.z /= nLen;

			Vec3 rest(remaining.x - dir.x * safe, 0.0f, remaining.z - dir.z * safe);
			const float into = rest.x * n.x + rest.z * n.z; // 벽을 파고드는 성분
			remaining.x = rest.x - n.x * into;
			remaining.z = rest.z - n.z * into;
			// 다음 반복에서 슬라이드 방향으로 재 sweep
		}
		else
		{
			result.x += remaining.x;
			result.z += remaining.z;
			break;
		}
	}

	return result;
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
		EnemyComponent* enemyComponent = mWorld->GetComponent<EnemyComponent>(entity);
		FlyComponent* flyComponent = mWorld->GetComponent<FlyComponent>(entity);

		if (!transformComponent || !enemyMovementComponent) continue;

		transformComponent->mMovingVector = enemyMovementComponent->mMovingDirection * dt * enemyMovementComponent->mMovingSpeed;
		transformComponent->mLocalPosition += transformComponent->mMovingVector;

		const bool skipNavSurface = enemyComponent &&
			enemyComponent->mEnemyType == EnemyType::Fly &&
			flyComponent && flyComponent->mDirectFlight;

		if (!skipNavSurface && nav && nav->IsInitialized())
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
	// 발밑 깊은 탐색 거리(낙하 착지 바닥 찾기).
	static constexpr float kFallProbe   = 5000.0f;	// 밑
	static constexpr float kAirStepUp   = 10.0f;	// 위

	// 발밑 레이가 일시적으로 빗나가도 직전 바닥을 유지하는 유예 시간
	static constexpr float kGroundGrace = 0.12f;

	auto physicsWorld = mWorld->GetPhysicsWorld();

	std::vector<Entity> gravityEntitys{ mWorld->GetEntitiesWithComponent<GravityComponent>() };
	for (auto& entity : gravityEntitys) {
		TransformComponent* transformComponent = mWorld->GetComponent<TransformComponent>(entity);
		GravityComponent* gravityComponent = mWorld->GetComponent<GravityComponent>(entity);
		if (!transformComponent || !gravityComponent)
			continue;

		// 낙사 사망 연출 중인 플레이어는 떨어진 자리에 고정
		if (auto* deadPlayer = mWorld->GetComponent<MainPlayerComponent>(entity);
			deadPlayer && deadPlayer->IsDeathActive())
			continue;

		const float feetY      = gravityComponent->mHight;
		const float prevGround = gravityComponent->mGround;
		const float stepDown   = gravityComponent->mStepDownDistance;
		// 점프 상승도 공중으로 취급
		const bool airborne = gravityComponent->mFalling || gravityComponent->mGravity < 0.0f;

		// === 발밑 지면 탐색 ===
		//  - 공중: 착지 바닥 탐색
		//  - 지상: 오를 수 있는 단 높이부터 탐색해서 같은 경로로 단차/계단 판정
		const float probeStepUp = airborne ? kAirStepUp : stepDown;
		float meshGround = 0.0f;
		bool found = false;
		if (physicsWorld)
			found = physicsWorld->TryQueryTerrainHeightNear(
				transformComponent->mLocalPosition, feetY, probeStepUp, kFallProbe, meshGround);

		if (found)
		{
			gravityComponent->mGround = meshGround;
			gravityComponent->mGroundGraceLeft = kGroundGrace;
		}
		else // 레이 실패
		{
			if (gravityComponent->mGroundGraceLeft > 0.0f)
			{	//직전 바닥 유지(메시 이음새/삼각형 틈으로 떨어짐 방지)
				gravityComponent->mGroundGraceLeft -= dt;
				gravityComponent->mGround = prevGround;
			}
			else
			{	// feetY 한참 아래를 바닥으로 둬 자연 낙하
				gravityComponent->mGround = feetY - kFallProbe;
			}
		}

		// === 수직 적분 ===
		if (airborne)
		{
			gravityComponent->mFalling = true;
			gravityComponent->mGravity += gravityComponent->mGravityA * dt;
			gravityComponent->mHight   -= gravityComponent->mGravity * dt; // mGravity<0 이면 상승

			// 이번 프레임에 바닥을 지나쳤으면 스냅해 착지.
			if (gravityComponent->mHight <= gravityComponent->mGround)
			{
				gravityComponent->mHight    = gravityComponent->mGround;
				gravityComponent->mGravity  = 0.0f;
				gravityComponent->mFalling  = false;
				gravityComponent->mDropping = false;


				// 착지시 다음 단차 판정을 새로 시작
				gravityComponent->mDropConfirmLeft = gravityComponent->mDropConfirmDelay;
			}
		}
		else
		{
			const float dist = gravityComponent->mHight - gravityComponent->mGround; // >0: 지면 위
			if (dist > stepDown)	// 바닥이 단차 이상 꺼짐(낙하 후보)
			{
				// 바로 낙하 확정하면 계단 중간에 빠지므로, dist 초과가 mDropConfirmDelay 동안 지속될 때만 낙하로 확정
				gravityComponent->mDropConfirmLeft -= dt;
				if (gravityComponent->mDropConfirmLeft <= 0.0f)
				{
					// 지속된다 ==> 진짜 낭떠러지(걸어서 떨어지기)
					gravityComponent->mFalling  = true;
					gravityComponent->mDropping = true;
					gravityComponent->mGravity  = (std::max)(0.0f, gravityComponent->mGravity);
				}

				// 잠시후 내려가게끔함
			}
			else // // 단차 이내(오르막/내리막/평지)
			{
				// 지면에 붙어있기
				gravityComponent->mHight    = gravityComponent->mGround;
				gravityComponent->mGravity  = 0.0f;
				gravityComponent->mFalling  = false;
				gravityComponent->mDropping = false;

				// 정상 접지시 디바운스 타이머 리셋
				gravityComponent->mDropConfirmLeft = gravityComponent->mDropConfirmDelay;
			}
		}

		transformComponent->mLocalPosition.y = gravityComponent->mHight + 3.f;
	}
}

void MovementSystem::UpdateFlyHeight(float dt)
{
	(void)dt;

	auto physicsWorld = mWorld->GetPhysicsWorld();
	if (!physicsWorld || !mWorld->HasComponentPool<FlyComponent>())
		return;

	for (const Entity& entity : mWorld->GetEntitiesWithComponent<FlyComponent>())
	{
		TransformComponent* transformComponent = mWorld->GetComponent<TransformComponent>(entity);
		FlyComponent* flyComponent = mWorld->GetComponent<FlyComponent>(entity);
		EnemyComponent* enemyComponent = mWorld->GetComponent<EnemyComponent>(entity);
		if (!transformComponent || !flyComponent)
			continue;

		float ground = flyComponent->mGround;
		if (physicsWorld->TryQueryTerrainHeightNear(
			transformComponent->mLocalPosition,
			transformComponent->mLocalPosition.y,
			100.0f,
			5000.0f,
			ground))
		{
			flyComponent->mGround = ground;
		}

		const bool divingAttack = enemyComponent &&
			enemyComponent->mEnemyType == EnemyType::Fly &&
			enemyComponent->mAnimState == static_cast<uint8>(EnemyAnimState::Attack);
		if (divingAttack)
			continue;

		const float targetHoverHeight = flyComponent->mHoverHeight;
		float desiredY = flyComponent->mGround + targetHoverHeight + 3.0f;
		const float clearance = transformComponent->mLocalPosition.y - flyComponent->mGround;
		if (clearance < flyComponent->mMinGroundClearance)
		{
			desiredY = (std::max)(desiredY, flyComponent->mGround + flyComponent->mMinGroundClearance + 3.0f);
		}

		const float maxVerticalStep = flyComponent->mVerticalMoveSpeed * dt;
		const float deltaY = desiredY - transformComponent->mLocalPosition.y;
		if (fabsf(deltaY) <= maxVerticalStep)
			transformComponent->mLocalPosition.y = desiredY;
		else
			transformComponent->mLocalPosition.y += (deltaY > 0.0f ? maxVerticalStep : -maxVerticalStep);
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
