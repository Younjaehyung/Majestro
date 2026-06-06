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
#include "DeathCamComponent.h"
#include "LobbyRoomStateComponent.h"
#include "LobbyRoomListComponent.h"
#include "EventManager.h"
#include "GameEvents.h"
#include "Network.h"
#include "PauseMenuController.h"

PlayerInputSystem::PlayerInputSystem(World* world) : System(world)
{
	mPhase = SysPhase::Pre;
}

void PlayerInputSystem::Initialize()
{
}

void PlayerInputSystem::Update(float dt)
{
	PlayerInputContext ctx{};
	ctx.paused = IsPaused();

	UpdateDebugMouseLookInput(ctx);
	UpdateLobbyInput(dt, ctx);

	if (!BuildCameraContext(ctx))
		return;

	if (!mWorld->HasComponentPool<PlayerMovementComponent>())
		return;
	if (!mWorld->HasComponentPool<MainPlayerComponent>())
		return;

	UpdateCameraModeInput(ctx);

	if (!BuildPlayerContext(ctx))
		return;

	if (UpdateFreeCameraInput(dt, ctx))
		return;
	if (UpdatePausedInput(ctx))
		return;
	if (UpdateDeadInput(ctx))
		return;

	UpdateAliveInput(dt, ctx);
}

bool PlayerInputSystem::IsPaused() const
{
	if (!mWorld->HasComponentPool<PauseMenuController>())
		return false;

	auto pauseEntities = mWorld->GetEntitiesWithComponent<PauseMenuController>();
	if (pauseEntities.empty())
		return false;

	const PauseMenuController* pause = mWorld->GetComponent<PauseMenuController>(pauseEntities[0]);
	return pause != nullptr && pause->mPaused;
}

void PlayerInputSystem::ClearGameplayInput(PlayerInputContext& ctx)
{
	if (ctx.movement == nullptr || ctx.player == nullptr)
		return;

	ctx.movement->mMovingDirection = { 0, 0, 0 };
	ctx.movement->mJump = false;
	ctx.movement->mDash = false;
	ctx.movement->mAttack = false;
	ctx.movement->mSkill1 = false;
	ctx.movement->mSkill2 = false;
	ctx.movement->mReload = false;
	ctx.movement->mSpecial = false;
	ctx.player->mSpeed = 0.0f;
}

bool PlayerInputSystem::BuildCameraContext(PlayerInputContext& ctx)
{
	if (!mWorld->HasComponentPool<MainCameraComponent>())
		return false;

	std::vector<Entity> mainCameraEntities{ mWorld->GetEntitiesWithComponent<MainCameraComponent>() };
	if (mainCameraEntities.empty())
		return false;

	ctx.mainCameraEntity = mainCameraEntities[0];
	ctx.cameraType = mWorld->GetComponent<CameraTypeComponent>(ctx.mainCameraEntity);
	return true;
}

bool PlayerInputSystem::BuildPlayerContext(PlayerInputContext& ctx)
{
	if (!mWorld->HasComponentPool<PlayerMovementComponent>())
		return false;
	if (!mWorld->HasComponentPool<MainPlayerComponent>())
		return false;

	auto entities = mWorld->GetEntitiesWithComponents<
		PlayerMovementComponent,
		MainPlayerComponent,
		BeatComponent,
		LocalPlayerComponent>();

	if (entities.empty())
		return false;

	ctx.playerEntity = entities[0];
	ctx.movement = mWorld->GetComponent<PlayerMovementComponent>(ctx.playerEntity);
	ctx.player = mWorld->GetComponent<MainPlayerComponent>(ctx.playerEntity);
	ctx.beat = mWorld->GetComponent<BeatComponent>(ctx.playerEntity);

	return ctx.movement != nullptr && ctx.player != nullptr && ctx.beat != nullptr;
}

bool PlayerInputSystem::IsPlayerDead(const MainPlayerComponent* player) const
{
	if (player == nullptr)
		return false;

	return player->mLowerState == static_cast<int>(ReplicatedMovementMode::Dead) ||
		player->mUpperState == static_cast<int>(ReplicatedActionState::Dead);
}

void PlayerInputSystem::UpdateDebugMouseLookInput(const PlayerInputContext& ctx)
{
	if (!ctx.paused && INPUT.GetKeyDown(eKeyCode::GRAVE))
		INPUT.SetForceMouseLook(!INPUT.IsMouseLookActive());
}

void PlayerInputSystem::UpdateLobbyInput(float, PlayerInputContext&)
{
	if (!mWorld->HasComponentPool<ChoicePlayerComponent>())
		return;

	std::vector<Entity> choiceEntities{ mWorld->GetEntitiesWithComponent<ChoicePlayerComponent>() };
	if (choiceEntities.empty())
		return;

	ChoicePlayerComponent* choice = mWorld->GetComponent<ChoicePlayerComponent>(choiceEntities[0]);
	if (choice == nullptr)
		return;

	bool characterChanged = false;
	if (INPUT.GetKeyDown(eKeyCode::LEFT))
	{
		choice->mPlayerType = (choice->mPlayerType + 2) % 3;
		characterChanged = true;
	}
	else if (INPUT.GetKeyDown(eKeyCode::RIGHT))
	{
		choice->mPlayerType = (choice->mPlayerType + 1) % 3;
		characterChanged = true;
	}

	if (characterChanged)
	{
		if (auto eventMgr = mWorld->GetEventManager())
			eventMgr->Enqueue(EvRoomCharacterChanged{ choice->mPlayerType });
	}

	bool inRoom = false;
	if (mWorld->HasComponentPool<LobbyRoomListComponent>())
	{
		auto listEntities = mWorld->GetEntitiesWithComponent<LobbyRoomListComponent>();
		if (!listEntities.empty())
		{
			auto* list = mWorld->GetComponent<LobbyRoomListComponent>(listEntities[0]);
			inRoom = (list != nullptr) && (list->mCurrentRoomId != 0);
		}
	}

	if (!inRoom || !INPUT.GetKeyDown(eKeyCode::R) || !mWorld->HasComponentPool<LobbyRoomStateComponent>())
		return;

	std::vector<Entity> roomStateEntities = mWorld->GetEntitiesWithComponent<LobbyRoomStateComponent>();
	if (roomStateEntities.empty())
		return;

	LobbyRoomStateComponent* state = mWorld->GetComponent<LobbyRoomStateComponent>(roomStateEntities[0]);
	const uint32 myClientId = Network::GetInstance().mClientId;
	bool currentReady = false;

	if (state != nullptr)
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

void PlayerInputSystem::UpdateCameraModeInput(PlayerInputContext& ctx)
{
	if (ctx.cameraType == nullptr)
		return;

	if (INPUT.GetKeyDown(eKeyCode::F1))
	{
		ctx.cameraType->mPlayMode = ONE_FPS;
	}
	else if (INPUT.GetKeyDown(eKeyCode::F2))
	{
		ctx.cameraType->mPlayMode = THREE_FPS;
	}
	else if (INPUT.GetKeyDown(eKeyCode::F3))
	{
		ctx.cameraType->mPlayMode = THREE_RPG;
	}
	else if (INPUT.GetKeyDown(eKeyCode::F4))
	{
		ctx.cameraType->mPlayMode = MAIN_CAMERA;
		ctx.cameraType->mFreeCamInit = false;
	}
}

bool PlayerInputSystem::UpdateFreeCameraInput(float, PlayerInputContext& ctx)
{
	if (ctx.cameraType == nullptr || ctx.cameraType->mPlayMode != MAIN_CAMERA)
		return false;

	TransformComponent* camTransform = mWorld->GetComponent<TransformComponent>(ctx.mainCameraEntity);

	if (!ctx.cameraType->mFreeCamInit && camTransform != nullptr)
	{
		ctx.cameraType->mFreeCamPos = camTransform->mLocalPosition;
		ctx.cameraType->mFreeYaw = camTransform->mLocalRotationE.y;
		ctx.cameraType->mFreePitch = camTransform->mLocalRotationE.x;
		ctx.cameraType->mFreeCamInit = true;
	}

	if (INPUT.IsMouseLookActive())
	{
		auto mouseDelta = INPUT.GetMouseState().Delta;
		float dPitch = (std::abs(mouseDelta.y) > deadzone) ? static_cast<float>(mouseDelta.y) : 0.0f;
		float dYaw = (std::abs(mouseDelta.x) > deadzone) ? static_cast<float>(mouseDelta.x) : 0.0f;
		ctx.cameraType->mFreePitch += dPitch * sensitivity * mDPI;
		ctx.cameraType->mFreeYaw += dYaw * sensitivity * mDPI;
		ctx.cameraType->mFreePitch = std::clamp(ctx.cameraType->mFreePitch, -85.0f, 85.0f);
	}


	ClearGameplayInput(ctx);
	INPUT.MouseStateClear();
	return true;
}

bool PlayerInputSystem::UpdatePausedInput(PlayerInputContext& ctx)
{
	if (!ctx.paused)
		return false;


	ClearGameplayInput(ctx);
	INPUT.MouseStateClear();
	return true;
}

bool PlayerInputSystem::UpdateDeadInput(PlayerInputContext& ctx)
{
	if (!IsPlayerDead(ctx.player))
		return false;

	
	if (DeathCamComponent* death = mWorld->GetComponent<DeathCamComponent>(ctx.mainCameraEntity))
	{
		if (INPUT.GetMouseLeftDown())
			death->mSpectateCycleReq = 1;
		else if (INPUT.GetMouseRightDown())
			death->mSpectateCycleReq = -1;
	}

	ClearGameplayInput(ctx);
	INPUT.MouseStateClear();
	return true;
}

void PlayerInputSystem::UpdateAliveInput(float dt, PlayerInputContext& ctx)
{
	if (!INPUT.GetKey(eKeyCode::W) && !INPUT.GetKey(eKeyCode::A) &&
		!INPUT.GetKey(eKeyCode::S) && !INPUT.GetKey(eKeyCode::D))
	{
		ctx.player->mSpeed = 0.f;
	}

	ctx.movement->mMovingDirection = { 0, 0, 0 };
	ctx.movement->mJump = INPUT.GetKey(eKeyCode::SPACE);
	ctx.movement->mDash = INPUT.GetKey(eKeyCode::SHIFT);

	const bool mouseLook = INPUT.IsMouseLookActive();
	ctx.movement->mAttack = mouseLook && INPUT.GetMouseState().LeftDown;
	ctx.movement->mSkill1 = INPUT.GetKey(eKeyCode::Q);
	ctx.movement->mSkill2 = INPUT.GetKey(eKeyCode::E);
	ctx.movement->mReload = INPUT.GetKey(eKeyCode::R);
	ctx.movement->mSpecial = mouseLook && INPUT.GetMouseRightDown();

	if (INPUT.GetMouseRightDown())
	{
		ctx.player->mNextRhythm = (ctx.player->mNextRhythm + 1) % 4;
		if (ctx.player->mNextRhythm != ctx.player->mRhythm)
			ctx.player->mHasQueuedRhythmChange = true;

		cout << "next rythm:" << static_cast<int>(ctx.player->mNextRhythm) << endl;
		mWorld->GetEventManager()->Enqueue(EvRhythmChanged{ ctx.player->mNextRhythm });
	}

	if (INPUT.GetKey(eKeyCode::A))
		ctx.movement->mMovingDirection.x -= 1;
	if (INPUT.GetKey(eKeyCode::W))
		ctx.movement->mMovingDirection.z += 1;
	if (INPUT.GetKey(eKeyCode::S))
		ctx.movement->mMovingDirection.z -= 1;
	if (INPUT.GetKey(eKeyCode::D))
		ctx.movement->mMovingDirection.x += 1;

	if (INPUT.GetKeyDown(eKeyCode::SPACE))
	{
		if (ctx.beat->mBouns)
			cout << "Hit Beat!" << endl;
		else
			cout << "fail" << endl;
	}

	if (INPUT.GetKey(eKeyCode::Q))
		ctx.movement->mMovingDirection.y -= 1;
	if (INPUT.GetKey(eKeyCode::E))
		ctx.movement->mMovingDirection.y += 1;

	if (INPUT.IsMouseLookActive())
	{
		auto mouseDelta = INPUT.GetMouseState().Delta;

		float deltaX = (std::abs(mouseDelta.y) > deadzone) ? static_cast<float>(mouseDelta.y) : 0.0f;
		float deltaY = (std::abs(mouseDelta.x) > deadzone) ? static_cast<float>(mouseDelta.x) : 0.0f;

		ctx.movement->mTargetRotation.x += deltaX * sensitivity * mDPI;
		ctx.movement->mTargetRotation.y += deltaY * sensitivity * mDPI;
		ctx.movement->mTargetRotation.x = std::clamp(ctx.movement->mTargetRotation.x, -60.0f, 60.0f);

		float alpha = std::clamp(dt * lerpFactor, 0.0f, 1.0f);

		ctx.movement->mCurrentRotation.x = std::lerp(ctx.movement->mCurrentRotation.x, ctx.movement->mTargetRotation.x, alpha);
		ctx.movement->mCurrentRotation.y = std::lerp(ctx.movement->mCurrentRotation.y, ctx.movement->mTargetRotation.y, alpha);

		ctx.movement->mCameraRotationX = ctx.movement->mCurrentRotation.x;
		ctx.movement->mCameraRotationY = ctx.movement->mCurrentRotation.y;
		ctx.movement->mCameraRotationX = std::clamp(ctx.movement->mCameraRotationX, -60.0f, 60.0f);
	}

	INPUT.MouseStateClear();
	ctx.player->Update(dt);
}




