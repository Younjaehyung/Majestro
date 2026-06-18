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
	bool UpdatePausedInput(PlayerInputContext& ctx);
	bool UpdateDeadInput(PlayerInputContext& ctx);
	void UpdateAliveInput(float dt, PlayerInputContext& ctx);
	void ClearGameplayInput(PlayerInputContext& ctx);
};
