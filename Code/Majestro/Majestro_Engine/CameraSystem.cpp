#include "pch.h"
#include "CameraSystem.h"
#include "Engine.h"
#include "RenderManager.h"
#include "CameraComponent.h"
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

			if (cameraTypeComponent->mPlayMode == ONE_FPS) {
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
			// 카메라 쉐이크: 공격 등의 임팩트 시 pitch를 sin 파형으로 진동
			if (cameraTypeComponent && cameraTypeComponent->mShakeRemaining > 0.f)
			{
				cameraTypeComponent->mShakeRemaining -= dt;
				cameraTypeComponent->mShakeTimeAcc   += dt;

				float decay      = cameraTypeComponent->mShakeRemaining / cameraTypeComponent->mShakeDuration;
				float shakeAngle = sinf(cameraTypeComponent->mShakeTimeAcc * cameraTypeComponent->mShakeFrequency * XM_2PI)
								   * cameraTypeComponent->mShakeMagnitude * decay;

				transformComponent->mLocalRotationE.x += shakeAngle;
			}

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

	Vec3 pivot = playerPos + worldOffset;
	Vec3 DestPos = pivot - camType->mCameraMaxLenth * look;

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

	float cameraDistance = camType->mCameraMaxLenth;
	if (hit)
	{
		cameraDistance = hitDist - camType->mCameraMargin;
		if (cameraDistance < camType->mCameraMinLenth)
			cameraDistance = camType->mCameraMinLenth;
		if (cameraDistance > camType->mCameraMaxLenth)
			cameraDistance = camType->mCameraMaxLenth;
	}

	transform->mLocalPosition = pivot - cameraDistance * look;

	if (transform->mLocalPosition.y < playerPos.y) {
		transform->mLocalPosition.y = playerPos.y;
	}

	// Camera Dithered Fade
	{
		float fadeStart = camType->mCameraFadeStart;
		float fadeEnd   = camType->mCameraFadeEnd;
		float targetAlpha = 1.f;
		if (fadeStart > fadeEnd + 0.001f)
		{
			float t = (cameraDistance - fadeEnd) / (fadeStart - fadeEnd);
			if (t < 0.f) t = 0.f;
			else if (t > 1.f) t = 1.f;
			targetAlpha = t;
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
