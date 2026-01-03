#include "pch.h"
#include "PlayerSystem.h"
#include "Engine.h"
#include "PlayerComponent.h"
#include "TransformComponent.h"
#include "CameraComponent.h"
#include "TagComponent.h"
#include "InputManager.h"
#include "TransformSystem.h"
#include "TerrainComponent.h"
#include "BeatComponent.h"


PlayerSystem::PlayerSystem(World* world) : System(world)
{
}

void PlayerSystem::Initialize()
{
}

void PlayerSystem::Update(float dt)
{
	std::vector<Entity> entitys{ mWorld->GetEntitiesWithComponents<ControllerComponent, TransformComponent>() };

	//mode change
	if (INPUT.GetKeyDown(eKeyCode::F1)) {
		std::vector<Entity> mainPlayerEntitys{ mWorld->GetEntitiesWithComponent<MainPlayerComponent>() };
		TransformComponent* t = mWorld->GetComponent<TransformComponent>(mainPlayerEntitys[0]);
		mWorld->RemoveComponent<ControllerComponent>(entitys[0]);
		mWorld->AddComponent<ControllerComponent>(mainPlayerEntitys[0], *t, ONE_FPS);
	}
	else if (INPUT.GetKeyDown(eKeyCode::F2)) {
		std::vector<Entity> mainPlayerEntitys{ mWorld->GetEntitiesWithComponent<MainPlayerComponent>() };
		TransformComponent* t = mWorld->GetComponent<TransformComponent>(mainPlayerEntitys[0]);
		mWorld->RemoveComponent<ControllerComponent>(entitys[0]);
		mWorld->AddComponent<ControllerComponent>(mainPlayerEntitys[0], *t, THREE_FPS);
	}
	else if (INPUT.GetKeyDown(eKeyCode::F3)) {
		std::vector<Entity> mainPlayerEntitys{ mWorld->GetEntitiesWithComponent<MainPlayerComponent>() };
		TransformComponent* t = mWorld->GetComponent<TransformComponent>(mainPlayerEntitys[0]);
		mWorld->RemoveComponent<ControllerComponent>(entitys[0]);
		mWorld->AddComponent<ControllerComponent>(mainPlayerEntitys[0], *t, THREE_RPG);
	}
	else if (INPUT.GetKeyDown(eKeyCode::F4)) {
		std::vector<Entity> mainCameraEntitys{ mWorld->GetEntitiesWithComponent<MainCameraComponent>() };
		TransformComponent* t = mWorld->GetComponent<TransformComponent>(mainCameraEntitys[0]);
		mWorld->RemoveComponent<ControllerComponent>(entitys[0]);
		mWorld->AddComponent<ControllerComponent>(mainCameraEntitys[0], *t, MAIN_CAMERA);
	}


	else {

		TransformComponent* transformComponent = mWorld->GetComponent<TransformComponent>(entitys[0]);
		ControllerComponent* controllerComponent = mWorld->GetComponent<ControllerComponent>(entitys[0]);
		MainPlayerComponent* mainPlayerComponent = mWorld->GetComponent<MainPlayerComponent>(entitys[0]);
		BeatComponent* beatComponent = mWorld->GetComponent<BeatComponent>(entitys[0]);

		Input(dt, controllerComponent, mainPlayerComponent, beatComponent->mBouns);

		if (controllerComponent->mPlayMode == MAIN_CAMERA) {
			transformComponent->mLocalPosition = controllerComponent->mTransformComponent.mLocalPosition;
			transformComponent->mLocalRotation = controllerComponent->mTransformComponent.mLocalRotation;
		}
		else if (controllerComponent->mPlayMode == ONE_FPS || controllerComponent->mPlayMode == THREE_FPS) {
			transformComponent->mLocalPosition.x = controllerComponent->mTransformComponent.mLocalPosition.x;
			transformComponent->mLocalPosition.z = controllerComponent->mTransformComponent.mLocalPosition.z;
			transformComponent->mLocalPosition.y = mainPlayerComponent->mHight;
			transformComponent->mLocalRotation.y = controllerComponent->mTransformComponent.mLocalRotation.y;
			//std::cout << mainPlayerComponent->GetState() << std::endl;
			//std::cout << mainPlayerComponent->mHight << std::endl;
			mainPlayerComponent->Update(dt);
		}
		else if (controllerComponent->mPlayMode == THREE_RPG) {
			transformComponent->mLocalPosition.x = controllerComponent->mTransformComponent.mLocalPosition.x;
			transformComponent->mLocalPosition.z = controllerComponent->mTransformComponent.mLocalPosition.z;
			
			//transformComponent->mLocalRotation.y = controllerComponent->mTransformComponent.mLocalRotation.y;
		}

		auto& terrains = mWorld->GetComponentPool<TerrainComponent>();
		auto terrainEntities = mWorld->GetEntitiesWithComponent<TerrainComponent>();
		TerrainComponent* terrainComponent = mWorld->GetComponent<TerrainComponent>(terrainEntities[0]);
		float terrainHeight = terrainComponent->GetHeightAtWorldPosition(controllerComponent->mTransformComponent.mLocalPosition);
		mainPlayerComponent->mGround = terrainHeight;
		

		transformComponent->FinalUpdate();

		controllerComponent->mTransformComponent.mLocalPosition = transformComponent->mLocalPosition;
		controllerComponent->mTransformComponent.FinalUpdate();

		//TestUpdate(dt);
		for (auto& entity : entitys) {
			//CameraComponent* cameraComponent = mWorld->GetComponent<CameraComponent>(entity);
			//TransformComponent* transformComponent = mWorld->GetComponent<TransformComponent>(entity);

			//transformComponent->FinalUpdate();
			//cameraComponent->FinalUpdate(transformComponent->GetLocalToWorldMatrix().Invert());
		}
	}
}

void PlayerSystem::Input(float dt, ControllerComponent* controllerComponent, MainPlayerComponent* mainPlayerComponent, bool beatHit)
{


	if (!INPUT.GetKey(eKeyCode::W) && !INPUT.GetKey(eKeyCode::A) && !INPUT.GetKey(eKeyCode::S) && !INPUT.GetKey(eKeyCode::D)) {
		mainPlayerComponent->mSpeed = 0.f;
		mainPlayerComponent->mFsm.ChangeState(mainPlayerComponent, IdleState::Instance());
	}
	else {
		if (mainPlayerComponent->GetState() & S_Dash)mainPlayerComponent->mSpeed = 200.f;
		else mainPlayerComponent->mSpeed = 90.0f;
	}

	if (INPUT.GetKey(eKeyCode::A)) {
		mainPlayerComponent->mFsm.ChangeState(mainPlayerComponent, WalkState::Instance());
		controllerComponent->mTransformComponent.mLocalPosition -= controllerComponent->mTransformComponent.GetRight() * dt * mainPlayerComponent->mSpeed;
	}
	if (INPUT.GetKey(eKeyCode::W)) {
		mainPlayerComponent->mFsm.ChangeState(mainPlayerComponent, WalkState::Instance());
		controllerComponent->mTransformComponent.mLocalPosition += controllerComponent->mTransformComponent.GetLook() * dt * mainPlayerComponent->mSpeed;
	}
	if (INPUT.GetKey(eKeyCode::S)) {
		mainPlayerComponent->mFsm.ChangeState(mainPlayerComponent, WalkState::Instance());
		controllerComponent->mTransformComponent.mLocalPosition -= controllerComponent->mTransformComponent.GetLook() * dt * mainPlayerComponent->mSpeed;
	}
	if (INPUT.GetKey(eKeyCode::D)) {
		mainPlayerComponent->mFsm.ChangeState(mainPlayerComponent, WalkState::Instance());
		controllerComponent->mTransformComponent.mLocalPosition += controllerComponent->mTransformComponent.GetRight() * dt * mainPlayerComponent->mSpeed;
	}



	if (INPUT.GetKeyDown(eKeyCode::SPACE)) {
		if (beatHit) cout << "Hit Beat!" << endl;
		else cout << "fail" << endl;

		mainPlayerComponent->mFsm.ChangeState(mainPlayerComponent, JumpState::Instance());
	}
	if (INPUT.GetKeyDown(eKeyCode::SHIFT)) {
		mainPlayerComponent->mFsm.ChangeState(mainPlayerComponent, DashState::Instance());
	}


	if (INPUT.GetKey(eKeyCode::Q)) {
		controllerComponent->mTransformComponent.mLocalPosition -= controllerComponent->mTransformComponent.GetUp() * dt * speed;
	}
	if (INPUT.GetKey(eKeyCode::E)) {
		controllerComponent->mTransformComponent.mLocalPosition += controllerComponent->mTransformComponent.GetUp() * dt * speed;
	}


	if (INPUT.GetMouseState().LeftDown) {
		//attack

		//screen move
		controllerComponent->mTransformComponent.mLocalRotation.x += (float)INPUT.GetMouseState().Delta.y * dt * DPI;
		controllerComponent->mTransformComponent.mLocalRotation.y += (float)INPUT.GetMouseState().Delta.x * dt * DPI;
		INPUT.MouseStateClear();
	}
	
}