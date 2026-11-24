#include "pch.h"
#include "McpInput.h"
#include "Engine.h"
#include "Timer.h"
#include "SceneManager.h"
#include "Scene.h"
#include "TransformComponent.h"
#include "TagComponent.h"
#include "MovementComponent.h"

namespace McpInputDetail
{
	// eKeyCode 순서 그대로 (InputManager.h)
	constexpr const char* kKeyNames[] = {
		"Q", "W", "E", "R", "T", "Y", "U", "I", "O", "P",
		"A", "S", "D", "F", "G", "H", "J", "K", "L",
		"Z", "X", "C", "V", "B", "N", "M", "LEFT", "RIGHT", "DOWN", "UP", "SPACE", "SHIFT",
		"0", "1", "2", "3", "4", "5", "6", "7", "8", "9",
		"F1", "F2", "F3", "F4", "F5", "F6", "F7", "F8",
		"ESC", "TAB", "GRAVE",
		"NUMPAD0", "NUMPAD1", "NUMPAD2", "NUMPAD3", "NUMPAD4",
		"NUMPAD5", "NUMPAD6", "NUMPAD7", "NUMPAD8", "NUMPAD9",
	};
	static_assert(std::size(kKeyNames) == static_cast<size_t>(eKeyCode::End), "eKeyCode 와 이름표가 어긋남");

	int FindKey(std::string name)
	{
		for (char& c : name)
			c = static_cast<char>(::toupper(static_cast<unsigned char>(c)));
		for (size_t i = 0; i < std::size(kKeyNames); ++i)
			if (name == kKeyNames[i])
				return static_cast<int>(i);
		return -1;
	}

	float FlatDistance(const Vec3& a, const Vec3& b)
	{
		const float dx = a.x - b.x;
		const float dz = a.z - b.z;
		return std::sqrt(dx * dx + dz * dz);
	}
}

McpInput& McpInput::Get()
{
	static McpInput sInstance;
	return sInstance;
}

void McpInput::Finish(const char* result)
{
	mMode = Mode::None;
	mResult = result;
}

json McpInput::Press(const json& request)
{
	InputFrame press{};
	for (const std::string& key : request.value("keys", std::vector<std::string>{}))
	{
		const int index = McpInputDetail::FindKey(key);
		if (index < 0)
			return { { "ok", false }, { "error", "unknown key: " + key } };
		press.keys[index] = true;
	}
	press.mouseLeft = request.value("mouse_left", false);
	press.mouseRight = request.value("mouse_right", false);
	press.mouseDelta = { request.value("mouse_dx", 0L), request.value("mouse_dy", 0L) };

	mPress = press;
	mDuration = (std::clamp)(request.value("seconds", 0.1f), 0.f, 30.f);
	mElapsed = 0.f;
	mMode = Mode::Press;
	mResult = "running";
	return { { "ok", true } };
}

json McpInput::MoveTo(const json& request)
{
	const auto& pos = request.at("pos");
	mTarget = Vec3(pos.at(0).get<float>(), pos.at(1).get<float>(), pos.at(2).get<float>());
	mArrive = (std::max)(30.f, request.value("arrive_cm", 150.f));
	mTimeout = (std::clamp)(request.value("timeout", 30.f), 1.f, 300.f);
	mElapsed = 0.f;
	mDistance = -1.f;
	mNav.ResetPath();
	mMode = Mode::Move;
	mResult = "running";
	return { { "ok", true } };
}

json McpInput::Cancel()
{
	if (mMode != Mode::None)
		Finish("cancelled");
	return Status();
}

json McpInput::Status() const
{
	json status = { { "ok", true }, { "active", mMode != Mode::None }, { "result", mResult },
		{ "elapsed", mElapsed } };
	if (mDistance >= 0.f)
		status["distance"] = mDistance;
	return status;
}

void McpInput::Apply(InputFrame& frame)
{
	if (mMode == Mode::None)
		return;

	const float dt = gEngine->GetTimer().GetTimeElapsed();
	mElapsed += dt;

	if (mMode == Mode::Press)
	{
		frame = mPress;
		if (mElapsed >= mDuration)
			Finish("done");	// 이번 프레임까지 누르고, 다음 프레임에 뗀다
		return;
	}

	// ---- move_to ----
	frame = InputFrame{};	// QaBot 입력이 있었다면 덮어쓴다
	auto scene = gEngine->GetSceneManager().GetActiveScene();
	World* world = scene ? scene->GetWorld().get() : nullptr;
	if (world == nullptr || !world->HasComponentPool<LocalPlayerComponent>())
		return Finish("no player");

	auto players = world->GetEntitiesWithComponents<TransformComponent, LocalPlayerComponent>();
	const PlayerMovementComponent* movement = players.empty() || !world->HasComponentPool<PlayerMovementComponent>()
		? nullptr : world->GetComponent<PlayerMovementComponent>(players[0]);
	if (movement == nullptr)
		return Finish("no player");

	if (mNavScene != scene->GetSceneId())
	{
		mNavScene = scene->GetSceneId();
		mNav.Load(mNavScene);	// NavMesh 가 없는 씬(광장 등)은 직선으로 간다
	}

	const Vec3 position = world->GetComponent<TransformComponent>(players[0])->mWorldPosition;
	mDistance = McpInputDetail::FlatDistance(position, mTarget);
	if (mDistance <= mArrive)
		return Finish("arrived");
	if (mElapsed >= mTimeout)
		return Finish("timeout");

	mNav.Steer(frame, *movement, position, mTarget, dt, true);
}
