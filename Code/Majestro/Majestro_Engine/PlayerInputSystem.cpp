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
#include "IntroSequenceComponent.h"
#include "LobbyRoomStateComponent.h"
#include "LobbyRoomListComponent.h"
#include "EventManager.h"
#include "GameEvents.h"
#include "RhythmEmissiveComponent.h"
#include "Network.h"
#include "PauseMenuController.h"
#include "NpcComponent.h"
#include "RenderManager.h"

namespace
{
	// 월드 좌표를 화면 픽셀로 투영. 카메라 뒤/화면 밖이면 false.
	bool ProjectWorldToScreen(const Vec3& worldPos, const Matrix& view, const Matrix& proj,
		float vpW, float vpH, Vec2& out)
	{
		const DirectX::XMVECTOR wp = DirectX::XMVectorSet(worldPos.x, worldPos.y, worldPos.z, 1.f);
		const DirectX::XMVECTOR vp = DirectX::XMVector4Transform(wp, view);
		const DirectX::XMVECTOR cp = DirectX::XMVector4Transform(vp, proj);

		DirectX::XMFLOAT4 clip;
		DirectX::XMStoreFloat4(&clip, cp);
		if (clip.w <= 0.0001f)
			return false;

		const float ndcX = clip.x / clip.w;
		const float ndcY = clip.y / clip.w;
		out.x = (ndcX * 0.5f + 0.5f) * vpW;
		out.y = (1.f - (ndcY * 0.5f + 0.5f)) * vpH;

		if (ndcX < -1.2f || ndcX > 1.2f || ndcY < -1.2f || ndcY > 1.2f)
			return false;
		return true;
	}
}

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

	if (UpdateCinematicInput(ctx))
		return;
	if (UpdateFreeCameraInput(dt, ctx))
		return;
	if (UpdateDialogueInput(ctx))
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

bool PlayerInputSystem::IsCinematicPlaying() const
{
	if (!mWorld->HasComponentPool<IntroSequenceComponent>())
		return false;

	const IntroSequenceComponent* seq =
		mWorld->GetComponent<IntroSequenceComponent>(mWorld->GetSingletonEntity());
	return seq != nullptr && seq->mPlaying;
}

bool PlayerInputSystem::IsDialogueActive() const
{
	const DialogueStateComponent* dialogue = mWorld->GetSingleton<DialogueStateComponent>();
	return dialogue != nullptr && (dialogue->mActive || dialogue->mLevelSelectActive);
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

bool PlayerInputSystem::IsPlayerAirborneForRhythmChange(const MainPlayerComponent* player) const
{
	if (player == nullptr)
		return false;

	return player->mLowerState == static_cast<int>(ReplicatedMovementMode::Airborne) ||
		player->mLowerState == static_cast<int>(ReplicatedMovementMode::Falling) ||
		player->mLowerState == static_cast<int>(ReplicatedMovementMode::Landing);
}

bool PlayerInputSystem::IsLobbyCharacterLockedByOtherPlayer(uint8 playerType) const
{

	if (!mWorld->HasComponentPool<LobbyRoomStateComponent>())
		return false;

	std::vector<Entity> roomStateEntities = mWorld->GetEntitiesWithComponent<LobbyRoomStateComponent>();
	if (roomStateEntities.empty())
		return false;

	const LobbyRoomStateComponent* state = mWorld->GetComponent<LobbyRoomStateComponent>(roomStateEntities[0]);
	if (state == nullptr || !state->mHasSnapshot)
		return false;

	const uint32 myClientId = Network::GetInstance().mClientId;
	const uint8 count = (state->mPlayerCount < ROOM_MAX_PLAYERS) ? state->mPlayerCount : ROOM_MAX_PLAYERS;
	for (uint8 i = 0; i < count; ++i)
	{
		const auto& slot = state->mSlots[i];
		if (slot.sessionId != 0 && slot.sessionId != myClientId &&
			slot.ready && slot.playerType == playerType)
		{
			return true;
		}
	}

	return false;
}

int PlayerInputSystem::PickLobbyCharacterByMouse() const
{
	if (!mWorld->HasComponentPool<MannequinComponent>())  return -1;
	if (!mWorld->HasComponentPool<MainCameraComponent>()) return -1;

	auto cams = mWorld->GetEntitiesWithComponent<MainCameraComponent>();
	if (cams.empty()) return -1;
	CameraComponent* cam = mWorld->GetComponent<CameraComponent>(cams[0]);
	if (!cam) return -1;

	const WindowInfo& window = RENDERMANAGER.GetWindow();
	const float vpW = static_cast<float>(window.Width);
	const float vpH = static_cast<float>(window.Height);

	const MouseState& mouse = INPUT.GetMouseState();
	const Vec2 mousePos{ static_cast<float>(mouse.Position.x), static_cast<float>(mouse.Position.y) };

	const Matrix view = cam->GetViewMatrix();
	const Matrix proj = cam->GetProjectionMatrix();

	// 캐릭터 클릭 히트박스(화면 픽셀).
	constexpr float kTorsoUpOffset = 90.f;	//  발 원점이라 몸통 높이로 올려 투영함
	constexpr float kHitHalfW = 110.f;
	constexpr float kHitHalfH = 200.f;

	int   best = -1;
	float bestDist = FLT_MAX;
	for (Entity e : mWorld->View<MannequinComponent>())
	{
		MannequinComponent* mann = mWorld->GetComponent<MannequinComponent>(e);
		TransformComponent*  tr  = mWorld->GetComponent<TransformComponent>(e);
		if (!mann || !tr) continue;

		Vec3 worldPos = tr->mLocalPosition;
		worldPos.y += kTorsoUpOffset;

		Vec2 sp{};
		if (!ProjectWorldToScreen(worldPos, view, proj, vpW, vpH, sp))
			continue;

		const float dx = mousePos.x - sp.x;
		const float dy = mousePos.y - sp.y;
		if (std::fabs(dx) > kHitHalfW || std::fabs(dy) > kHitHalfH)
			continue;

		const float d2 = dx * dx + dy * dy;
		if (d2 < bestDist)
		{
			bestDist = d2;
			best = static_cast<int>(mann->mPlayerType);
		}
	}
	return best;
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

	// 마우스 좌클릭으로 캐릭터 선택 ( 픽킹 )
	if (INPUT.GetMouseLeftDown())
	{
		const int picked = PickLobbyCharacterByMouse();
		if (picked >= 0)
		{
			const uint8 candidate = static_cast<uint8>(picked);
			if (candidate != choice->mPlayerType && !IsLobbyCharacterLockedByOtherPlayer(candidate))
			{
				choice->mPlayerType = candidate;
				characterChanged = true;
			}
		}
	}

		if (INPUT.GetKeyDown(eKeyCode::LEFT))
		{
			const uint8 currentType = choice->mPlayerType;
			uint8 candidateType = currentType;
			for (uint8 tryCount = 0; tryCount < 3; ++tryCount)
			{
				candidateType = static_cast<uint8>((candidateType + 1) % 3);
				if (!IsLobbyCharacterLockedByOtherPlayer(candidateType))
				{
					if (candidateType != currentType)
					{
					choice->mPlayerType = candidateType;
					characterChanged = true;
				}
				break;
			}
		}
	}
		else if (INPUT.GetKeyDown(eKeyCode::RIGHT))
		{
			const uint8 currentType = choice->mPlayerType;
			uint8 candidateType = currentType;
			for (uint8 tryCount = 0; tryCount < 3; ++tryCount)
			{
				candidateType = static_cast<uint8>((candidateType + 2) % 3);
				if (!IsLobbyCharacterLockedByOtherPlayer(candidateType))
				{
					if (candidateType != currentType)
					{
					choice->mPlayerType = candidateType;
					characterChanged = true;
				}
				break;
			}
		}
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

bool PlayerInputSystem::UpdateCinematicInput(PlayerInputContext& ctx)
{
	// 씬 진입 시네마틱 재생 중에는 게임플레이/카메라 입력을 완전히 잠금
	if (!IsCinematicPlaying())
		return false;

	ClearGameplayInput(ctx);
	INPUT.MouseStateClear();
	return true;
}

bool PlayerInputSystem::UpdateDialogueInput(PlayerInputContext& ctx)
{
	// 광장 NPC 대화 중에는 게임플레이 입력을 잠금
	if (!IsDialogueActive())
		return false;

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
	ctx.movement->mDance1 = INPUT.GetKeyDown(eKeyCode::_1);

	// 벽 스티커
	if (INPUT.GetKeyDown(eKeyCode::T))
	{
		if (CameraComponent* cam = mWorld->GetComponent<CameraComponent>(ctx.mainCameraEntity))
		{
			Matrix invView = cam->GetViewMatrix().Invert();
			Vec3 camPos = Vec3::Transform(Vec3::Zero, invView);
			Vec3 forward = Vec3::Transform(Vec3(0.0f, 0.0f, 1.0f), invView) - camPos;
			forward.Normalize();
			mWorld->GetEventManager()->Enqueue(EvStickerRequest{ camPos, forward, 100.0f, 0u });
		}
	}

	if (INPUT.GetMouseRightDown() && !IsPlayerAirborneForRhythmChange(ctx.player))
	{
		// 즉시 송신하지 않고 원하는 최종 리듬만 누적
		ctx.player->mDesiredRhythm = NextRhythm(ctx.player->mDesiredRhythm);
		ctx.player->mRhythmSettleTimer = MainPlayerComponent::kRhythmSettleTime;

		cout << "desired rythm:" << static_cast<int>(ctx.player->mDesiredRhythm) << endl;

		mWorld->GetEventManager()->Enqueue(EvRhythmChanged{ ctx.player->mDesiredRhythm });

		// 리듬 변경 이미시브
		auto* rhythmEmissive = mWorld->GetComponent<RhythmEmissiveComponent>(ctx.playerEntity);
		rhythmEmissive->mTimer = rhythmEmissive->mDuration;
	}

	if (INPUT.GetKey(eKeyCode::A))
		ctx.movement->mMovingDirection.x -= 1;
	if (INPUT.GetKey(eKeyCode::W))
		ctx.movement->mMovingDirection.z += 1;
	if (INPUT.GetKey(eKeyCode::S))
		ctx.movement->mMovingDirection.z -= 1;
	if (INPUT.GetKey(eKeyCode::D))
		ctx.movement->mMovingDirection.x += 1;

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




