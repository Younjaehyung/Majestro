#pragma once
#include "InputManager.h"
#include "NavSteer.h"

// MCP 가 넣는 입력: press(키·마우스를 일정 시간), move_to(NavMesh 경로로 좌표까지 걷기).
//   InputManager 입력 소스 끝에서 Apply 가 frame 을 덮어쓴다. 진행 중인 명령이 없으면 frame 을 건드리지 않는다.
//   입력 소스를 쓰면 하드웨어 입력이 꺼지므로, -mcpinput 또는 -qa 로 띄웠을 때만 켜진다 (배선은 Game.cpp).
//   -qa 와 같이 쓰면 MCP 명령이 진행되는 동안만 QaBot 입력을 덮어쓴다 (QaBot 판단은 계속 돈다).
//   모든 함수는 게임 스레드(McpBridge::Poll, InputManager::Update)에서만 불린다.
class McpInput
{
public:
	static McpInput& Get();

	void Enable() { mEnabled = true; }
	bool Enabled() const { return mEnabled; }

	void Apply(InputFrame& frame);

	// 요청 JSON 을 받아 명령을 시작하고, 시작 결과를 돌려준다. 진행 중인 명령은 새 명령으로 바뀐다.
	json Press(const json& request);
	json MoveTo(const json& request);
	json Cancel();
	json Status() const;

private:
	enum class Mode { None, Press, Move };

	void Finish(const char* result);

	bool mEnabled = false;
	Mode mMode = Mode::None;
	std::string mResult = "idle";
	float mElapsed = 0.f;

	// press
	InputFrame mPress;
	float mDuration = 0.f;

	// move_to
	NavSteer mNav;
	SceneId mNavScene = SceneId::End;
	Vec3 mTarget{};
	float mArrive = 150.f;
	float mTimeout = 30.f;
	float mDistance = -1.f;
};
