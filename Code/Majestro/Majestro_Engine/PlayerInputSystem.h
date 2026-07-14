#pragma once
#include "World.h"
#include "System.h"

class BeatComponent;
class CameraTypeComponent;
class MainPlayerComponent;
class PlayerMovementComponent;

class PlayerInputSystem : public System
{
public:
	PlayerInputSystem(World* world);

	void Initialize();
	void Update(float dt);

public:
	const float mDPI = 5.f;

	const float sensitivity = 0.1f;
	const float lerpFactor = 15.0f;
	const float deadzone = 0.001f;

private:
	struct PlayerInputContext
	{
		Entity mainCameraEntity{};
		Entity playerEntity{};
		CameraTypeComponent* cameraType = nullptr;
		PlayerMovementComponent* movement = nullptr;
		MainPlayerComponent* player = nullptr;
		BeatComponent* beat = nullptr;
		bool paused = false;
	};

	bool IsPaused() const;
	bool IsCinematicPlaying() const;	// 씬 진입 시네마틱(IntroSequence) 재생 중 여부
	bool IsDialogueActive() const;		// NPC 대화 진행 중 여부
	bool BuildCameraContext(PlayerInputContext& ctx);
	bool BuildPlayerContext(PlayerInputContext& ctx);
	bool IsPlayerDead(const MainPlayerComponent* player) const;
	bool IsPlayerAirborneForRhythmChange(const MainPlayerComponent* player) const;
	bool IsLobbyCharacterLockedByOtherPlayer(uint8 playerType) const;		// 다른 사람에 의해 이미 선택된건지 확인
	int  PickLobbyCharacterByMouse() const;									// 픽킹

	void UpdateDebugMouseLookInput(const PlayerInputContext& ctx);
	void UpdateLobbyInput(float dt, PlayerInputContext& ctx);
	void UpdateCameraModeInput(PlayerInputContext& ctx);
	bool UpdateFreeCameraInput(float dt, PlayerInputContext& ctx);
	bool UpdateCinematicInput(PlayerInputContext& ctx);		// 시네마틱 중 입력 완전 잠금
	bool UpdateDialogueInput(PlayerInputContext& ctx);		// 대화 중 게임플레이 입력 잠금
	bool UpdatePausedInput(PlayerInputContext& ctx);
	bool UpdateDeadInput(PlayerInputContext& ctx);
	void UpdateAliveInput(float dt, PlayerInputContext& ctx);
	bool UpdateEmoteWheelInput(float dt);
	void ResetEmoteWheelState();
	void ClearActionInput(PlayerInputContext& ctx);
	void ClearGameplayInput(PlayerInputContext& ctx);
};
