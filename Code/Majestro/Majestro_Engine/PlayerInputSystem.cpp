#include "pch.h"
#include "PlayerInputSystem.h"

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
#include "MovementComponent.h"
#include "LobbyRoomStateComponent.h"
#include "EventManager.h"
#include "GameEvents.h"
#include "Network.h"

PlayerInputSystem::PlayerInputSystem(World* world) : System(world)
{
	mPhase = SysPhase::Pre;
}

void PlayerInputSystem::Initialize()
{
}

void PlayerInputSystem::Update(float dt)
{
	// ` 키로 인게임 카메라 조작 | ImGui 디버그 조작 토글
	if (INPUT.GetKeyDown(eKeyCode::GRAVE))
	{
		INPUT.SetForceMouseLook(!INPUT.IsMouseLookActive());
	}

	//camera setting
	if (false == mWorld->HasComponentPool<MainCameraComponent>())return;
	
	std::vector<Entity> mainCameraEntitys{ mWorld->GetEntitiesWithComponent<MainCameraComponent>() };
	CameraTypeComponent* cameraTypeComponent = mWorld->GetComponent<CameraTypeComponent>(mainCameraEntitys[0]);


	if (mWorld->HasComponentPool<ChoicePlayerComponent>()) {
		std::vector<Entity> choiceEntitys{ mWorld->GetEntitiesWithComponent<ChoicePlayerComponent>() };
		ChoicePlayerComponent* choicecomponent = mWorld->GetComponent<ChoicePlayerComponent>(choiceEntitys[0]);
		bool characterChanged = false;
		if (INPUT.GetKeyDown(eKeyCode::LEFT))
		{
			choicecomponent->mPlayerType = (choicecomponent->mPlayerType + 2) % 3;
			characterChanged = true;
		}
		else if (INPUT.GetKeyDown(eKeyCode::RIGHT))
		{
			choicecomponent->mPlayerType = (choicecomponent->mPlayerType + 1) % 3;
			characterChanged = true;
		}

		// 캐릭터 변경을 서버에 알린다 (서버에서 ready 자동 해제)
		if (characterChanged)
		{
			if (auto eventMgr = mWorld->GetEventManager())
				eventMgr->Enqueue(EvRoomCharacterChanged{ choicecomponent->mPlayerType });
		}

		// R 키 Ready 토글. 본인 슬롯 ready 값을 읽어서 반전 후 enqueue.
		// LobbyRoomStateComponent 가 아직 스냅샷을 못 받았으면 true 로 시작.
		if (INPUT.GetKeyDown(eKeyCode::R) && mWorld->HasComponentPool<LobbyRoomStateComponent>())
		{
			std::vector<Entity> roomStateEntities = mWorld->GetEntitiesWithComponent<LobbyRoomStateComponent>();
			if (!roomStateEntities.empty())
			{
				LobbyRoomStateComponent* state = mWorld->GetComponent<LobbyRoomStateComponent>(roomStateEntities[0]);
				const uint32 myClientId = Network::GetInstance().mClientId;
				bool currentReady = false;
				if (state)
				{
					for (const auto& slot : state->mSlots)
					{
						if (slot.sessionId != 0 && slot.sessionId == myClientId)
						{
							currentReady = slot.ready;
							break;
						}
					}
				}
				if (auto eventMgr = mWorld->GetEventManager())
					eventMgr->Enqueue(EvRoomReadyChanged{ !currentReady });
			}
		}
	}

	if (false == mWorld->HasComponentPool<PlayerMovementComponent>())return;
	if (false == mWorld->HasComponentPool<MainPlayerComponent>())return;


	if (INPUT.GetKeyDown(eKeyCode::F1)) {
		cameraTypeComponent->mPlayMode = ONE_FPS;
	}
	else if (INPUT.GetKeyDown(eKeyCode::F2)) {
		cameraTypeComponent->mPlayMode = THREE_FPS;
	}
	else if (INPUT.GetKeyDown(eKeyCode::F3)) {
		cameraTypeComponent->mPlayMode = THREE_RPG;
	}
	else if (INPUT.GetKeyDown(eKeyCode::F4)) {
		cameraTypeComponent->mPlayMode = MAIN_CAMERA;
	}

	//player move

	auto entitys = mWorld->GetEntitiesWithComponents<
		PlayerMovementComponent,
		MainPlayerComponent,
		BeatComponent,
		LocalPlayerComponent>();

	if (entitys.empty())
		return;


	PlayerMovementComponent* movementComponent = mWorld->GetComponent<PlayerMovementComponent>(entitys[0]);
	MainPlayerComponent* mainPlayerComponent = mWorld->GetComponent<MainPlayerComponent>(entitys[0]);
	BeatComponent* beatComponent = mWorld->GetComponent<BeatComponent>(entitys[0]);


	if (!INPUT.GetKey(eKeyCode::W) && !INPUT.GetKey(eKeyCode::A) && !INPUT.GetKey(eKeyCode::S) && !INPUT.GetKey(eKeyCode::D)) {
		mainPlayerComponent->mSpeed = 0.f;
		//mainPlayerComponent->mFsm.ChangeState(mainPlayerComponent, IdleState::Instance());
	}
	else {
		//if (mainPlayerComponent->GetState() == S_Dash)mainPlayerComponent->mSpeed = mainPlayerComponent->mDashSpeed;
		//else mainPlayerComponent->mSpeed = mainPlayerComponent->mRunSpeed;
	}

	movementComponent->mMovingDirection = { 0,0,0 };
	movementComponent->mJump = INPUT.GetKey(eKeyCode::SPACE);
	movementComponent->mDash = INPUT.GetKey(eKeyCode::SHIFT);
	// 디버그 모드(마우스 룩 비활성)에서는 마우스 클릭을 게임에 반영하지 않음
	const bool mouseLook = INPUT.IsMouseLookActive();
	movementComponent->mAttack = mouseLook && INPUT.GetMouseState().LeftDown;
	movementComponent->mSkill1 = INPUT.GetKey(eKeyCode::Q);
	movementComponent->mSkill2 = INPUT.GetKey(eKeyCode::E);
	movementComponent->mReload = INPUT.GetKey(eKeyCode::R);
	movementComponent->mSpecial = mouseLook && INPUT.GetMouseRightDown();


	if (INPUT.GetMouseRightDown())
	{
		mainPlayerComponent->mNextRhythm = (mainPlayerComponent->mNextRhythm + 1) % 4;
		if (mainPlayerComponent->mNextRhythm != mainPlayerComponent->mRhythm)
			mainPlayerComponent->mHasQueuedRhythmChange = true;

		cout << "next rythm:" << (int)mainPlayerComponent->mNextRhythm << endl;
		mWorld->GetEventManager()->Enqueue(EvRhythmChanged{ mainPlayerComponent->mNextRhythm });
	}


	if (INPUT.GetKey(eKeyCode::A)) {
		movementComponent->mMovingDirection.x -= 1;
	}
	if (INPUT.GetKey(eKeyCode::W)) {
		movementComponent->mMovingDirection.z += 1;
	}
	if (INPUT.GetKey(eKeyCode::S)) {
		movementComponent->mMovingDirection.z -= 1;
	}
	if (INPUT.GetKey(eKeyCode::D)) {
		movementComponent->mMovingDirection.x += 1;
	}

	if (INPUT.GetKeyDown(eKeyCode::SPACE)) {
		if (beatComponent->mBouns) cout << "Hit Beat!" << endl;
		else cout << "fail" << endl;
		
		//mainPlayerComponent->mFsm.ChangeState(mainPlayerComponent, JumpState::Instance());
		//movementComponent->mJump = true;
		
	}
	if (INPUT.GetKeyDown(eKeyCode::SHIFT)) {
		//mainPlayerComponent->mFsm.ChangeState(mainPlayerComponent, DashState::Instance());

	}

	

	if (INPUT.GetKey(eKeyCode::Q)) {
		movementComponent->mMovingDirection.y -= 1;
	}
	if (INPUT.GetKey(eKeyCode::E)) {
		movementComponent->mMovingDirection.y += 1;
	}




	if (INPUT.IsMouseLookActive()) {
		//attack


		auto mouseDelta = INPUT.GetMouseState().Delta;

		// 미세한 노이즈 제거
		float deltaX = (std::abs(mouseDelta.y) > deadzone) ? (float)mouseDelta.y : 0.0f;
		float deltaY = (std::abs(mouseDelta.x) > deadzone) ? (float)mouseDelta.x : 0.0f;

		movementComponent->mTargetRotation.x += deltaX * sensitivity * mDPI;
		movementComponent->mTargetRotation.y += deltaY * sensitivity * mDPI;
		movementComponent->mTargetRotation.x = std::clamp(movementComponent->mTargetRotation.x, -60.0f, 60.0f);

		float alpha = std::clamp(dt * lerpFactor, 0.0f, 1.0f);
		

		movementComponent->mCurrentRotation.x = std::lerp(movementComponent->mCurrentRotation.x, movementComponent->mTargetRotation.x, alpha);
		movementComponent->mCurrentRotation.y = std::lerp(movementComponent->mCurrentRotation.y, movementComponent->mTargetRotation.y, alpha);


		movementComponent->mCameraRotationX = movementComponent->mCurrentRotation.x;
		movementComponent->mCameraRotationY = movementComponent->mCurrentRotation.y;

		// 짐벌락 방지 클램핑 (Pitch)
		movementComponent->mCameraRotationX = std::clamp(movementComponent->mCameraRotationX, -60.0f, 60.0f);
	}


		////screen move
		//movementComponent->mCameraRotationX += (float)INPUT.GetMouseState().Delta.y * dt * mDPI;
		//movementComponent->mCameraRotationY += (float)INPUT.GetMouseState().Delta.x * dt * mDPI;
		INPUT.MouseStateClear();
	

	mainPlayerComponent->Update(dt);

}
