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
	{"F_NONE", 1ull << 0},
	{"F_MOVE", 1ull << 1} ,{"F_STUN", 1ull << 2}, {"F_DEAD", 1ull << 3},{"F_JUMP", 1ull << 4}, {"F_SA", 1ull << 5}, {"F_INVUL", 1ull <<6},
	{"F_NO_RUN", 1ull <<7}
};

enum : StateId { S_Idle = 0, S_Walk = 1, S_Run = 2, S_Jump = 3, S_Dash = 4, 
	S_Aim = 5, S_ReRoad = 6, S_RhythmChange =7,
	S_Hit = 8, S_Stun = 9, S_Dead =10,
	S_Attack1 = 11, S_Attack2 = 12, S_Skill1 = 13, S_Skill2 = 14, S_Special =15, 

};

enum PlayerFlags : uint64_t
{
	FLAG_NONE = 1ull << 0, //상시 꺼짐 전이 불가 조건
	FLAG_MOVE = 1ull << 1,
	FLAG_STUN = 1ull << 2,
	FLAG_DEAD = 1ull << 3,
	FLAG_JUMP = 1ull << 4,
	FLAG_SA	=	1ull << 5, 
	FLAG_INVUL =1ull << 6, 
	FLAG_NO_RUN=1ull << 7,
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
	float mHight = 0.0f; //플레이어 높이
	float mGround = 0.0f;
};

//player base --------------------------------------------------
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

//battle support------------------------------------------------
class AimState : public State<MainPlayerComponent> {
public:
	static AimState* Instance();
	void Enter(MainPlayerComponent* owner) override;
	void Update(MainPlayerComponent* owner) override;
	void Exit(MainPlayerComponent* owner) override;
};
class ReRoadState : public State<MainPlayerComponent> {
public:
	static ReRoadState* Instance();
	void Enter(MainPlayerComponent* owner) override;
	void Update(MainPlayerComponent* owner) override;
	void Exit(MainPlayerComponent* owner) override;
};
class RhythmChangeState : public State<MainPlayerComponent> {
public:
	static RhythmChangeState* Instance();
	void Enter(MainPlayerComponent* owner) override;
	void Update(MainPlayerComponent* owner) override;
	void Exit(MainPlayerComponent* owner) override;
};

//damage--------------------------------------------------------
class HitState : public State<MainPlayerComponent> {
public:
	static HitState* Instance();
	void Enter(MainPlayerComponent* owner) override;
	void Update(MainPlayerComponent* owner) override;
	void Exit(MainPlayerComponent* owner) override;
};
class StunState : public State<MainPlayerComponent> {
public:
	static StunState* Instance();
	void Enter(MainPlayerComponent* owner) override;
	void Update(MainPlayerComponent* owner) override;
	void Exit(MainPlayerComponent* owner) override;
};
class DeadState : public State<MainPlayerComponent> {
public:
	static DeadState* Instance();
	void Enter(MainPlayerComponent* owner) override;
	void Update(MainPlayerComponent* owner) override;
	void Exit(MainPlayerComponent* owner) override;
};

//attack--------------------------------------------------------
class Attack1State : public State<MainPlayerComponent> {
public:
	static Attack1State* Instance();
	void Enter(MainPlayerComponent* owner) override;
	void Update(MainPlayerComponent* owner) override;
	void Exit(MainPlayerComponent* owner) override;
};
class Attack2State : public State<MainPlayerComponent> {
public:
	static Attack2State* Instance();
	void Enter(MainPlayerComponent* owner) override;
	void Update(MainPlayerComponent* owner) override;
	void Exit(MainPlayerComponent* owner) override;
};
class Skill1State : public State<MainPlayerComponent> {
public:
	static Skill1State* Instance();
	void Enter(MainPlayerComponent* owner) override;
	void Update(MainPlayerComponent* owner) override;
	void Exit(MainPlayerComponent* owner) override;
};
class Skill2State : public State<MainPlayerComponent> {
public:
	static Skill2State* Instance();
	void Enter(MainPlayerComponent* owner) override;
	void Update(MainPlayerComponent* owner) override;
	void Exit(MainPlayerComponent* owner) override;
};
class SpecialState : public State<MainPlayerComponent> {
public:
	static SpecialState* Instance();
	void Enter(MainPlayerComponent* owner) override;
	void Update(MainPlayerComponent* owner) override;
	void Exit(MainPlayerComponent* owner) override;
};

