#pragma once
#include "Component.h"
#include "TransformComponent.h"
#include "StateMachine.h"
#include "System.h"



enum PlayMode
{
	MAIN_CAMERA,
	ONE_FPS,
	THREE_FPS,
	THREE_RPG,
};

class ControllerComponent : public Component<ControllerComponent>
{
public:
	ControllerComponent() : mTransformComponent(), mPlayMode(MAIN_CAMERA) {}
	ControllerComponent(TransformComponent transform): mTransformComponent(transform), mPlayMode(MAIN_CAMERA) {}
	ControllerComponent(TransformComponent transform, PlayMode mode) : mTransformComponent(transform), mPlayMode(mode) {}
public:
	TransformComponent mTransformComponent;
	PlayMode mPlayMode;
	float mHight = 1; 
	float mCameraLenth = 5; 
};

//------------------------------------------------------------------------------------------------

static std::unordered_map<std::string, uint64_t> gFlagByName = {
	{"F_MOVE", 1ull << 0} ,{"F_STUN", 1ull << 1}, {"F_DEAD", 1ull << 2},{"F_JUMP", 1ull << 3}, {"F_SA", 1ull << 4}, {"F_INVUL", 1ull <<5},
	{"F_NO_RUN", 1ull <<6}
};

enum : StateId { S_Idle = 0, S_Walk = 1, S_Run = 2, S_Jump = 3, S_Dash = 4, S_Aim = 5 };

enum PlayerFlags : uint64_t
{
	FLAG_MOVE = 1ull << 0,
	FLAG_STUN = 1ull << 1,
	FLAG_DEAD = 1ull << 2,
	FLAG_JUMP = 1ull << 3,
	FLAG_SA	=	1ull << 4, 
	FLAG_INVUL =1ull << 5, 
	FLAG_NO_RUN=1ull << 6,
};

inline void SetFlag(uint64_t& f, uint64_t m) { f |= m; }   // 켜기
inline void ClearFlag(uint64_t& f, uint64_t m) { f &= ~m; }   // 끄기

class MainPlayerComponent : public Component<MainPlayerComponent>
{
public:
	MainPlayerComponent();
	MainPlayerComponent(const std::string& path);

	void StateCheck();
	void Update(float dt);
	uint32 GetState() { return (uint32)mFsm.GetState(); };

	void InitFSMOnce();
	void InitFSMFromJson(const std::string& path);

public:
	StateMachine<MainPlayerComponent> mFsm{this};
	float mSpeed = 0.0f;
	uint64_t mFlags = 0ull;
	float mStateTime=0.0f;
	float mDt=0.0f;

	float mGravity = 0.0f;
	float mGravityA = 0.98f; //중력가속도
	float mJumpPower = 150.f;
	float mHight = 0.0f;
	float mGround = 0.0f;
};

class IdleState : public State<MainPlayerComponent> {
public:
	static IdleState* Instance();
	void Enter(MainPlayerComponent* owner) override;
	void Update(MainPlayerComponent* owner) override;
	void Exit(MainPlayerComponent* owner) override;
};

class WalkState : public State<MainPlayerComponent> {
public:
	static WalkState* Instance();
	void Enter(MainPlayerComponent* owner) override;
	void Update(MainPlayerComponent* owner) override;
	void Exit(MainPlayerComponent* owner) override;
};

class RunState : public State<MainPlayerComponent> {
public:
	static RunState* Instance();
	void Enter(MainPlayerComponent* owner) override;
	void Update(MainPlayerComponent* owner) override;
	void Exit(MainPlayerComponent* owner) override;
};

class JumpState : public State<MainPlayerComponent> {
public:
	static JumpState* Instance();
	void Enter(MainPlayerComponent* owner) override;
	void Update(MainPlayerComponent* owner) override;
	void Exit(MainPlayerComponent* owner) override;
};

class DashState : public State<MainPlayerComponent> {
public:
	static DashState* Instance();
	void Enter(MainPlayerComponent* owner) override;
	void Update(MainPlayerComponent* owner) override;
	void Exit(MainPlayerComponent* owner) override;
};