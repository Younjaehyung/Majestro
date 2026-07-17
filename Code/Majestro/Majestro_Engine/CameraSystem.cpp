#include "pch.h"
#include "CameraSystem.h"
#include "MathUtils.h"
#include "Engine.h"
#include "RenderManager.h"
#include "CameraComponent.h"
#include "DeathCamComponent.h"
#include "TransformComponent.h"
#include "PlayerComponent.h"
#include "InputManager.h"
#include "TransformSystem.h"
#include "TagComponent.h"
#include "MovementComponent.h"
#include "RenderComponent.h"


CameraSystem::CameraSystem(World* world) : System(world)
{
}

void CameraSystem::Initialize()
{
}
	
void CameraSystem::Update(float dt)
{
	if (false == mWorld->HasComponentPool<MainCameraComponent>())return;

	std::vector<Entity> entitys{ mWorld->GetEntitiesWithComponent<CameraComponent>() };
	if (entitys.empty()) return;
	//TestUpdate(dt);

	for (auto& entity : entitys) {
		CameraComponent* cameraComponent = mWorld->GetComponent<CameraComponent>(entity);
		TransformComponent* transformComponent = mWorld->GetComponent<TransformComponent>(entity);
		CameraTypeComponent* cameraTypeComponent = mWorld->GetComponent<CameraTypeComponent>(entity);

		if (cameraTypeComponent) {

			auto& playerPosPool = mWorld->GetComponentPool<TransformComponent>();
			TransformComponent* playerPos = playerPosPool.GetComponent(cameraTypeComponent->mTargetID);

			auto& playerMovePool = mWorld->GetComponentPool<PlayerMovementComponent>();
			PlayerMovementComponent* movementComponent = playerMovePool.GetComponent(cameraTypeComponent->mTargetID);

			Vec3 pos = playerPos->mLocalPosition;

			auto& renderPool = mWorld->GetComponentPool<RenderComponent>();
			RenderComponent* targetRender = renderPool.GetComponent(cameraTypeComponent->mTargetID);

			// 사망 카메라 연출 처리 
			auto& playerPool = mWorld->GetComponentPool<MainPlayerComponent>();
			MainPlayerComponent* targetPlayer = playerPool.GetComponent(cameraTypeComponent->mTargetID);
			DeathCamComponent* death = mWorld->GetComponent<DeathCamComponent>(entity);
			const bool isDead = targetPlayer &&
				(targetPlayer->mLowerState == static_cast<int>(ReplicatedMovementMode::Dead) ||
				 targetPlayer->mUpperState == static_cast<int>(ReplicatedActionState::Dead));

			if (death)
			{
				if (!isDead)
				{
					// 낙사 판정 지점
					const bool grounded = targetPlayer &&
						(targetPlayer->mLowerState == static_cast<int>(ReplicatedMovementMode::Idle)     ||
						 targetPlayer->mLowerState == static_cast<int>(ReplicatedMovementMode::Grounded) ||
						 targetPlayer->mLowerState == static_cast<int>(ReplicatedMovementMode::Landing)  ||
						 targetPlayer->mLowerState == static_cast<int>(ReplicatedMovementMode::Dashing));
					if (grounded)
					{
						death->mGroundedAnchorPos = transformComponent->mLocalPosition;
						death->mHasGroundedAnchor = true;
					}
					// 부활 시 데스캠 리셋
					if (death->mActive)
					{
						death->mActive         = false;
						death->mSpectating     = false;
						death->mElapsed        = 0.f;
						death->mSpectateTarget = 0;
					}
				}
				else if (!death->mActive && cameraTypeComponent->mPlayMode != MAIN_CAMERA)
				{
					// 사망 (자유시점 중에는 데스캠 진입 안 함)
					death->mActive         = true;
					death->mElapsed        = 0.f;
					death->mSpectating     = false;
					death->mSpectateTarget = 0;
					death->mWasFall        = (playerPos->mLocalPosition.y < death->mFallDeathY);

					if (death->mWasFall && death->mHasGroundedAnchor)
						death->mCamPos = death->mGroundedAnchorPos; // 낙사: 마지막 지면
					else
						death->mCamPos = transformComponent->mLocalPosition;      // 일반: 현재 카메라 위치
				}
			}

			if (death && death->mActive) {
				UpdateDeathCamera(cameraTypeComponent, death, transformComponent, playerPos, dt);
			}
			else if (cameraTypeComponent->mPlayMode == ONE_FPS) {
				// F1 1인칭: 카메라를 머리 위치(yaw 공간 고정 오프셋)에 배치
				transformComponent->mLocalRotationE.x = movementComponent->mCameraRotationX;
				transformComponent->mLocalRotationE.y = movementComponent->mCameraRotationY;
				transformComponent->FinalUpdate();

				Vec3 look = transformComponent->GetLook();
				Vec3 yawFwd = look;
				yawFwd.y = 0.f;
				if (yawFwd.LengthSquared() <= 0.0001f)
					yawFwd = Vec3::Forward;
				else
					yawFwd.Normalize();

				Vec3 yawRight = Vec3::Up.Cross(yawFwd);
				if (yawRight.LengthSquared() <= 0.0001f)
					yawRight = transformComponent->GetRight();
				else
					yawRight.Normalize();

				transformComponent->mLocalPosition = pos
					+ yawRight * cameraTypeComponent->mFpsHeadOffset.x
					+ Vec3::Up * cameraTypeComponent->mFpsHeadOffset.y
					+ yawFwd  * cameraTypeComponent->mFpsHeadOffset.z;

				// 1인칭에서는 자기 모델을 완전히 숨겨 화면을 가리지 않게 한다.
				if (targetRender)
					targetRender->mOpacity = 0.f;
				cameraTypeComponent->mCurrentFadeAlpha = 0.f;
			}
			else if (cameraTypeComponent->mPlayMode == THREE_FPS) {
				UpdateOrbitCamera(cameraTypeComponent, transformComponent, pos, movementComponent, dt);
			}
			else if (cameraTypeComponent->mPlayMode == THREE_RPG) {
				// F3 RPG: 카메라 궤도는 3인칭과 동일(몸 회전은 MovementSystem에서 진행방향으로 처리)
				UpdateOrbitCamera(cameraTypeComponent, transformComponent, pos, movementComponent, dt);
			}
			else { // MAIN_CAMERA: 자유 비행 카메라
				UpdateFreeCamera(cameraTypeComponent, transformComponent, dt);
				// 자유 카메라에서는 플레이어 모델을 정상 표시
				if (targetRender)
					targetRender->mOpacity = 1.f;
				cameraTypeComponent->mCurrentFadeAlpha = 1.f;
			}




			// 카메라 쉐이크
			if(!(death && death->mActive))
				CameraShake(cameraTypeComponent, transformComponent, dt);

			transformComponent->FinalUpdate();
		}

		// 메인 카메라만 화면 종횡비 동기화 (그림자 카메라는 4096×4096 유지)
		if (mWorld->GetComponent<MainCameraComponent>(entity) != nullptr)
		{
			const WindowInfo& wnd = RENDERMANAGER.GetWindow();
			cameraComponent->SetWidth(static_cast<float>(wnd.Width));
			cameraComponent->SetHeight(static_cast<float>(wnd.Height));
		}

		cameraComponent->FinalUpdate(transformComponent->GetWorldMatrix().Invert());

	}

}




void CameraSystem::UpdateOrbitCamera(CameraTypeComponent* camType, TransformComponent* transform,
	const Vec3& playerPos, PlayerMovementComponent* movement, float dt)
{
	// 카메라 회전은 마우스 룩(카메라 yaw/pitch)을 따른다 — 몸 방향과 무관.
	transform->mLocalRotationE.x = movement->mCameraRotationX;
	transform->mLocalRotationE.y = movement->mCameraRotationY;
	transform->FinalUpdate();

	// Camera Spring Arm
	Vec3 look = transform->GetLook();
	Vec3 yawForward = look;
	yawForward.y = 0.0f;

	if (yawForward.LengthSquared() <= 0.0001f)
		yawForward = Vec3::Forward;
	else
		yawForward.Normalize();

	Vec3 yawRight = Vec3::Up.Cross(yawForward);
	if (yawRight.LengthSquared() <= 0.0001f)
		yawRight = transform->GetRight();
	else
		yawRight.Normalize();

	Vec3 worldOffset = yawRight * camType->mOffset.x
		+ Vec3::Up * camType->mOffset.y
		+ yawForward * camType->mOffset.z;

	// 스킬 줌아웃 추가 거리 갱신 후 카메라 최대 거리에 반영
	UpdateDolly(camType, dt);
	const float maxLen = camType->mCameraMaxLenth + camType->mDollyOffset;

	Vec3 pivot = playerPos + worldOffset;
	Vec3 DestPos = pivot - maxLen * look;

	if (DestPos.y < playerPos.y)
		DestPos.y = playerPos.y;


	Vec3 sweepDir = DestPos - pivot;
	float maxDist = sweepDir.Length();
	if (maxDist > 1e-4f)
		sweepDir /= maxDist;
	else
		sweepDir = -look;

	auto physicsWorld = mWorld->GetPhysicsWorld();
	JoltStaticHit joltHit{};
	bool hasJoltHit = false;
	if (physicsWorld->HasJoltStaticCollision())
	{
		hasJoltHit = physicsWorld->CastMovingSphereAgainstStatic(
			pivot, DestPos,
			camType->mCameraSphereRadius, joltHit);
	}

	SweepHit obbHit{};
	if (!hasJoltHit)
	{
		obbHit = physicsWorld->SphereSweepVsOBB(pivot, DestPos,
			camType->mCameraSphereRadius);
	}

	const bool  hit     = hasJoltHit || obbHit.hit;
	const float hitDist = hasJoltHit ? joltHit.distance : obbHit.distance;

	float cameraDistance = maxDist;
	if (hit)
	{
		cameraDistance = hitDist - camType->mCameraMargin;
		if (cameraDistance < camType->mCameraMinLenth)
			cameraDistance = camType->mCameraMinLenth;
		if (cameraDistance > maxDist)
			cameraDistance = maxDist;
	}


	transform->mLocalPosition = pivot + cameraDistance * sweepDir;

	// Camera Dithered Fade
	{
		float fadeStart = camType->mCameraFadeStart;
		float fadeEnd   = camType->mCameraFadeEnd;
		float targetAlpha = 1.f;
		if (fadeStart > fadeEnd + 0.001f)
		{
			targetAlpha=  std::clamp((cameraDistance - fadeEnd) / (fadeStart - fadeEnd), 0.3f, 1.f);
		}

		float lerpFactor = camType->mFadeLerpSpeed * dt;
		if (lerpFactor > 1.f) lerpFactor = 1.f;
		camType->mCurrentFadeAlpha +=
			(targetAlpha - camType->mCurrentFadeAlpha) * lerpFactor;

		auto& renderPool = mWorld->GetComponentPool<RenderComponent>();
		if (RenderComponent* targetRender = renderPool.GetComponent(camType->mTargetID))
		{
			targetRender->mOpacity = camType->mCurrentFadeAlpha;
		}
	}
}


void CameraSystem::UpdateFreeCamera(CameraTypeComponent* camType, TransformComponent* transform, float dt)
{
	// 회전(mFreeYaw/mFreePitch)은 PlayerInputSystem에서 마우스로 누적. 여기서는 위치만 적분.
	transform->mLocalRotationE.x = camType->mFreePitch;
	transform->mLocalRotationE.y = camType->mFreeYaw;
	transform->mLocalRotationE.z = 0.f;
	transform->FinalUpdate();

	Vec3 forward = transform->GetLook();
	Vec3 right = transform->GetRight();

	float iz = (INPUT.GetKey(eKeyCode::W) ? 1.f : 0.f) - (INPUT.GetKey(eKeyCode::S) ? 1.f : 0.f);
	float ix = (INPUT.GetKey(eKeyCode::D) ? 1.f : 0.f) - (INPUT.GetKey(eKeyCode::A) ? 1.f : 0.f);
	float iy = (INPUT.GetKey(eKeyCode::E) || INPUT.GetKey(eKeyCode::SPACE) ? 1.f : 0.f)
			 - (INPUT.GetKey(eKeyCode::Q) || INPUT.GetKey(eKeyCode::SHIFT) ? 1.f : 0.f);

	Vec3 dir = forward * iz + right * ix + Vec3::Up * iy;
	if (dir.LengthSquared() > 0.0001f)
		dir.Normalize();

	camType->mFreeCamPos += dir * dt * camType->mFreeCamSpeed;
	transform->mLocalPosition = camType->mFreeCamPos;
}


void CameraSystem::UpdateDeathCamera(CameraTypeComponent* camType, DeathCamComponent* death,
	TransformComponent* transform, TransformComponent* selfPlayer, float dt)
{
	death->mElapsed += dt;

	// 사망 지점
	transform->mLocalPosition = death->mCamPos;

	// 자기 모델
	auto& renderPool = mWorld->GetComponentPool<RenderComponent>();
	if (RenderComponent* selfRender = renderPool.GetComponent(camType->mTargetID))
		selfRender->mOpacity = 1.f;
	camType->mCurrentFadeAlpha = 1.f;

	// 기본 응시 대상 (자기 캐릭터)
	Vec3 lookTarget = selfPlayer->mLocalPosition;
	lookTarget.y += death->mLookHeight;

	// 관전 대상
	if (death->mSpectating && death->mSpectateTarget != 0)
	{
		auto& tfPool = mWorld->GetComponentPool<TransformComponent>();
		if (TransformComponent* tp = tfPool.GetComponent(death->mSpectateTarget))
		{
			lookTarget = tp->mLocalPosition;
			lookTarget.y += death->mLookHeight;
		}
	}

	// 플레이어 바라보게끔함
	Vec3 dir = lookTarget - transform->mLocalPosition;
	if (dir.LengthSquared() > 1e-4f)
	{
		dir.Normalize();
		const float yawDeg   = MathUtils::YawDegreesFromDir(dir);
		const float pitchDeg = MathUtils::PitchDegreesFromDir(dir);
		transform->mLocalRotationE.x = pitchDeg;
		transform->mLocalRotationE.y = yawDeg;
		transform->mLocalRotationE.z = 0.f;
	}
	transform->FinalUpdate();
}

void CameraSystem::CameraShake(CameraTypeComponent* camType, TransformComponent* transform, float dt)
{
	if (camType->mShakeRemaining <= 0.f || camType->mShakeDuration <= 0.f)
		return;

	camType->mShakeRemaining -= dt;
	camType->mShakeTimeAcc += dt;

	float decay = camType->mShakeRemaining / camType->mShakeDuration;
	float t = camType->mShakeTimeAcc * camType->mShakeFrequency * XM_2PI;
	const Vec3& angles = camType->mShakeAngles;

	transform->mLocalRotationE.x += sinf(t) * angles.x * decay;
	transform->mLocalRotationE.y += sinf(t + 1.7f) * angles.y * decay;
	transform->mLocalRotationE.z += sinf(t + 3.4f) * angles.z * decay;
}

void CameraSystem::UpdateDolly(CameraTypeComponent* camType, float dt)
{
	// hold 동안은 target(뒤로 빠진 거리)으로 빠르게, 끝나면 0으로 천천히 보간
	float target;
	float speed;
	if (camType->mDollyRemaining > 0.f)
	{
		camType->mDollyRemaining -= dt;
		target = camType->mDollyTarget;
		speed  = camType->mDollyInSpeed;
	}
	else
	{
		target = 0.f;
		speed  = camType->mDollyOutSpeed;
	}

	float k = speed * dt;
	if (k > 1.f) k = 1.f;
	camType->mDollyOffset += (target - camType->mDollyOffset) * k;

	// 거의 복귀하면 0으로 스냅
	if (camType->mDollyRemaining <= 0.f && camType->mDollyOffset < 0.05f)
		camType->mDollyOffset = 0.f;
}


void CameraSystem::TestUpdate(float dt)
{
	std::vector<Entity> entitys{ mWorld->GetEntitiesWithComponents<CameraComponent, TransformComponent>() };
	TransformComponent* transformComponent = mWorld->GetComponent<TransformComponent>(entitys[0]);
	

	/*if (INPUT.GetKey(eKeyCode::A)) {
		transformComponent->mLocalPosition -= transformComponent->GetRight() * dt * 50.f;
	}
	if (INPUT.GetKey(eKeyCode::W)) {
		transformComponent->mLocalPosition += transformComponent->GetLook() * dt * 50.f;
	}
	if (INPUT.GetKey(eKeyCode::S)) {
		transformComponent->mLocalPosition -= transformComponent->GetLook() * dt * 50.f;
	}
	if (INPUT.GetKey(eKeyCode::D)) {
		transformComponent->mLocalPosition += transformComponent->GetRight()* dt * 50.f;
	}
	if (INPUT.GetKey(eKeyCode::Q)) {
		transformComponent->mLocalPosition -= transformComponent->GetUp() * dt * 50.f;
	}
	if (INPUT.GetKey(eKeyCode::E)) {
		transformComponent->mLocalPosition += transformComponent->GetUp() * dt * 50.f;
	}*/
	
	
	/*const float DPI = 0.5f;
	if (INPUT.GetMouseState().LeftDown) {
		transformComponent->mLocalRotation.x += (float)INPUT.GetMouseState().Delta.y * dt * DPI;
		transformComponent->mLocalRotation.y += (float)INPUT.GetMouseState().Delta.x * dt * DPI;
		INPUT.MouseStateClear();
	}*/
	
	
	
}
