#include "pch.h"
#include "PlayerSystem.h"
#include "PlayerComponent.h"
#include "TransformComponent.h"
#include "CameraComponent.h"
#include "TagComponent.h"
#include "InputManager.h"
#include "TransformSystem.h"
#include "TerrainComponent.h"
#include "BeatComponent.h"
#include "MovementComponent.h"


PlayerSystem::PlayerSystem(World* world) : System(world)
{
}

void PlayerSystem::Initialize()
{
}

void PlayerSystem::Update(float dt)
{
	//std::vector<Entity> entitys{ mWorld->GetEntitiesWithComponents<ControllerComponent, TransformComponent>() };

	//std::vector<Entity> mainCameraEntitys{ mWorld->GetEntitiesWithComponent<MainCameraComponent>() };
	//CameraTypeComponent* cameraTypeComponent = mWorld->GetComponent<CameraTypeComponent>(mainCameraEntitys[0]);

	////mode change
	//if (INPUT.GetKeyDown(eKeyCode::F1)) {
	//	cameraTypeComponent->mPlayMode = ONE_FPS;
	//}
	//else if (INPUT.GetKeyDown(eKeyCode::F2)) {
	//	cameraTypeComponent->mPlayMode = THREE_FPS;
	//}
	//else if (INPUT.GetKeyDown(eKeyCode::F3)) {
	//	cameraTypeComponent->mPlayMode = THREE_RPG;
	//}
	//else if (INPUT.GetKeyDown(eKeyCode::F4)) {
	//	cameraTypeComponent->mPlayMode = MAIN_CAMERA;
	//}
	//else {
	//	MovementComponent* movementComponent = mWorld->GetComponent<MovementComponent>(entitys[0]);
	//	TransformComponent* transformComponent = mWorld->GetComponent<TransformComponent>(entitys[0]);
	//	MainPlayerComponent* mainPlayerComponent = mWorld->GetComponent<MainPlayerComponent>(entitys[0]);
	//	BeatComponent* beatComponent = mWorld->GetComponent<BeatComponent>(entitys[0]);

	//	Input(dt, movementComponent, mainPlayerComponent, beatComponent->mBouns);

	//	if (cameraTypeComponent->mPlayMode == ONE_FPS || cameraTypeComponent->mPlayMode == THREE_FPS) {
	//		Vec3 forward = transformComponent->GetLook();
	//		Vec3 right = transformComponent->GetRight();

	//		// WASD 입력
	//		float ix = movementComponent->mMovingDirection.x;  // A/D  (-1 ~ 1)
	//		float iy = movementComponent->mMovingDirection.z;  // W/S   (-1 ~ 1)

	//		// 로컬 입력 방향을 월드 방향으로 변환
	//		Vec3 desired = forward * iy + right * ix;

	//		// 정규화
	//		if (desired.LengthSquared() > 0.0001f)
	//			desired.Normalize();

	//		transformComponent->mLocalPosition += desired * dt * mainPlayerComponent->mSpeed;

	//		transformComponent->mLocalRotation.y = movementComponent->mCameraRotationY;
	//		mainPlayerComponent->Update(dt);

	//	}
	//	else if (cameraTypeComponent->mPlayMode == THREE_RPG) {
	//		Vec3 forward = transformComponent->GetLook();
	//		Vec3 right = transformComponent->GetRight();

	//		// WASD 입력
	//		float ix = movementComponent->mMovingDirection.x;  // A/D  (-1 ~ 1)
	//		float iy = movementComponent->mMovingDirection.z;  // W/S   (-1 ~ 1)

	//		// 로컬 입력 방향을 월드 방향으로 변환
	//		Vec3 desired = forward * iy + right * ix;

	//		// 정규화
	//		if (desired.LengthSquared() > 0.0001f)
	//			desired.Normalize();

	//		if (mainPlayerComponent->mSpeed >0) {
	//			transformComponent->mLocalRotation.y = movementComponent->mCameraRotationY;
	//		}
	//		transformComponent->mLocalPosition += desired * dt * mainPlayerComponent->mSpeed;

	//		//
	//		mainPlayerComponent->Update(dt);
	//	}

	//	//transformComponent->FinalUpdate();

	//	//controllerComponent->mTransformComponent.mLocalPosition = transformComponent->mLocalPosition;
	//	//controllerComponent->mTransformComponent.FinalUpdate();


	//	for (auto& entity : entitys) {
	//		//CameraComponent* cameraComponent = mWorld->GetComponent<CameraComponent>(entity);
	//		//TransformComponent* transformComponent = mWorld->GetComponent<TransformComponent>(entity);

	//		//transformComponent->FinalUpdate();
	//		//cameraComponent->FinalUpdate(transformComponent->GetLocalToWorldMatrix().Invert());
	//	}
	//}
}

void PlayerSystem::Input(float dt, MovementComponent* movementComponent, MainPlayerComponent* mainPlayerComponent, bool beatHit)
{


	//if (!INPUT.GetKey(eKeyCode::W) && !INPUT.GetKey(eKeyCode::A) && !INPUT.GetKey(eKeyCode::S) && !INPUT.GetKey(eKeyCode::D)) {
	//	mainPlayerComponent->mSpeed = 0.f;
	//	mainPlayerComponent->mFsm.ChangeState(mainPlayerComponent, IdleState::Instance());
	//}
	//else {
	//	if (mainPlayerComponent->GetState() & S_Dash)mainPlayerComponent->mSpeed = mainPlayerComponent->mDashSpeed;
	//	else mainPlayerComponent->mSpeed = mainPlayerComponent->mRunSpeed;
	//}

	//movementComponent->mMovingDirection = { 0,0,0 };

	//if (INPUT.GetKey(eKeyCode::A)) {
	//	mainPlayerComponent->mFsm.ChangeState(mainPlayerComponent, WalkState::Instance());
	//	movementComponent->mMovingDirection.x -=1;
	//	}
	//if (INPUT.GetKey(eKeyCode::W)) {
	//	mainPlayerComponent->mFsm.ChangeState(mainPlayerComponent, WalkState::Instance());
	//	movementComponent->mMovingDirection.z += 1;
	//	}
	//if (INPUT.GetKey(eKeyCode::S)) {
	//	mainPlayerComponent->mFsm.ChangeState(mainPlayerComponent, WalkState::Instance());
	//	movementComponent->mMovingDirection.z -= 1;
	//	}
	//if (INPUT.GetKey(eKeyCode::D)) {
	//	mainPlayerComponent->mFsm.ChangeState(mainPlayerComponent, WalkState::Instance());
	//	movementComponent->mMovingDirection.x += 1;
	//	}



	//if (INPUT.GetKeyDown(eKeyCode::SPACE)) {
	//	if (beatHit) cout << "Hit Beat!" << endl;
	//	else cout << "fail" << endl;

	//	mainPlayerComponent->mFsm.ChangeState(mainPlayerComponent, JumpState::Instance());
	//}
	//if (INPUT.GetKeyDown(eKeyCode::SHIFT)) {
	//	mainPlayerComponent->mFsm.ChangeState(mainPlayerComponent, DashState::Instance());
	//}


	//if (INPUT.GetKey(eKeyCode::Q)) {
	//	movementComponent->mMovingDirection.y -= 1;
	//}
	//if (INPUT.GetKey(eKeyCode::E)) {
	//	movementComponent->mMovingDirection.y += 1;
	//}


	//if (INPUT.GetMouseState().LeftDown) {
	//	//attack

	//	//screen move
	//	movementComponent->mCameraRotationX += (float)INPUT.GetMouseState().Delta.y * dt * DPI;
	//	movementComponent->mCameraRotationY += (float)INPUT.GetMouseState().Delta.x * dt * DPI;
	//	INPUT.MouseStateClear();
	//}
	
}