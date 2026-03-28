#pragma once
#include "Component.h"
#include "TransformComponent.h"
#include "StateMachine.h"
#include "System.h"
#include "IMGUIComponent.h"
#include "Animator.h"

class ControllerComponent : public Component<ControllerComponent>
{
public:
	ControllerComponent() : mTransformComponent(){}
	ControllerComponent(TransformComponent transform): mTransformComponent(transform){}
	ControllerComponent(TransformComponent transform, int mode) : mTransformComponent(transform), mPlayMode(mode) {}

	void RegisterEditorProperties(std::vector<EditorProperty>& out)
	{
		out.push_back({ "Speed", PropertyType::Float, &mCameraHight, 0.0f, 10.0f });
		out.push_back({ "GodMode", PropertyType::Bool, &mCameraLenth });
	}

public:
	TransformComponent mTransformComponent;
	int mPlayMode;
	float mCameraHight = 1; 
	float mCameraLenth = 5; 
};

//------------------------------------------------------------------------------------------------

static std::unordered_map<std::string, uint64_t> gFlagByName = {
	{"F_NONE", 1ull << 0},
	{"F_MOVE", 1ull << 1} ,{"F_STUN", 1ull << 2}, {"F_DEAD", 1ull << 3},{"F_JUMP", 1ull << 4}, {"F_SA", 1ull << 5}, {"F_INVUL", 1ull <<6},
	{"F_NO_RUN", 1ull <<7}
};

enum : StateId {
	S_Idle = 0, S_RunForward, S_RunBackward, S_RunRight, S_RunLeft,
	S_Jump, S_Fall, S_Land, S_Dash,
	S_Attack1, S_Attack2, S_Skill1, S_Skill2, S_Special,
	S_Reload, S_RhythmChange, S_Aim,
	S_Hit, S_Stun, S_Dead,

};

enum PlayerFlags : uint64_t
{
	FLAG_NONE = 1ull << 0, //상시 꺼짐 전이 불가 조건
	FLAG_ANIM = 1ull << 1, 
	FLAG_MOVE = 1ull << 2,
	FLAG_STUN = 1ull << 3,
	FLAG_DEAD = 1ull << 4,
	FLAG_JUMP = 1ull << 5,
	FLAG_SA	=	1ull << 6, 
	FLAG_INVUL =1ull << 7, 
	FLAG_NO_RUN=1ull << 8,
};

inline void SetFlag(uint64_t& f, uint64_t m) { f |= m; }   // 켜기
inline void ClearFlag(uint64_t& f, uint64_t m) { f &= ~m; }   // 끄기

class MainPlayerComponent : public Component<MainPlayerComponent>
{
public:
	MainPlayerComponent();
	MainPlayerComponent(const std::string& path);
	MainPlayerComponent(const std::string& path, vector<shared_ptr<Animator>> anim);
	MainPlayerComponent(const std::string& path, vector<shared_ptr<Animator>> anim, uint8 playerType);

	void StateCheck();
	void Update(float dt);
	uint32 GetState() { return (uint32)mFsm.GetState(); };

	void InitFSMOnce();
	void InitFSMFromJson(const std::string& path);
	void LoadStateSettingFromJson(const std::string& path);

public:
	uint8 mPlayerType;
public:
	StateMachine<MainPlayerComponent> mFsm{this};
	int mNextState;
	int mStatePacket;
	int mLowerStatePacket;

	float mSpeed = 0.0f;
	float mWalkSpeed = 0.0f;
	float mRunSpeed = 0.0f;
	float mDashSpeed = 0.0f;
	float mJumpPower = 60.f;
	bool mFalling = false;

	uint64_t mFlags = 0ull;
	bool mAnimEnd = false;
	float mStateTime=0.0f;
	float mDt=0.0f;

	// 플레이어 custom 변수들
	float mDashTime = 0.0f;

	int mNowBullet = 0;
	int mMaxBullet = 0;

	uint8 mRhythm = 0;
	uint8 mNextRhythm = 0;
	bool mHasQueuedRhythmChange = false;
};

//player base --------------------------------------------------
class IdleState : public State<MainPlayerComponent> {
public:
	static IdleState* Instance();
	virtual const char* GetName() const override { return "IdleState"; }
	void Enter(MainPlayerComponent* owner) override;
	void Update(MainPlayerComponent* owner) override;
	void Exit(MainPlayerComponent* owner) override;
};
class WalkState : public State<MainPlayerComponent> {
public:
	static WalkState* Instance();
	virtual const char* GetName() const override { return "WalkState"; }
	void Enter(MainPlayerComponent* owner) override;
	void Update(MainPlayerComponent* owner) override;
	void Exit(MainPlayerComponent* owner) override;
};
class RunState : public State<MainPlayerComponent> {
public:
	static RunState* Instance();
	virtual const char* GetName() const override { return "RunState"; }
	void Enter(MainPlayerComponent* owner) override;
	void Update(MainPlayerComponent* owner) override;
	void Exit(MainPlayerComponent* owner) override;
};
class JumpState : public State<MainPlayerComponent> {
public:
	static JumpState* Instance();
	virtual const char* GetName() const override { return "JumpState"; }
	void Enter(MainPlayerComponent* owner) override;
	void Update(MainPlayerComponent* owner) override;
	void Exit(MainPlayerComponent* owner) override;
};
class FallState : public State<MainPlayerComponent> {
public:
	static FallState* Instance();
	virtual const char* GetName() const override { return "FallState"; }
	void Enter(MainPlayerComponent* owner) override;
	void Update(MainPlayerComponent* owner) override;
	void Exit(MainPlayerComponent* owner) override;
};
class LandState : public State<MainPlayerComponent> {
public:
	static LandState* Instance();
	virtual const char* GetName() const override { return "LandState"; }
	void Enter(MainPlayerComponent* owner) override;
	void Update(MainPlayerComponent* owner) override;
	void Exit(MainPlayerComponent* owner) override;
};
class DashState : public State<MainPlayerComponent> {
public:
	static DashState* Instance();
	virtual const char* GetName() const override { return "DashState"; }
	void Enter(MainPlayerComponent* owner) override;
	void Update(MainPlayerComponent* owner) override;
	void Exit(MainPlayerComponent* owner) override;
};

//battle support------------------------------------------------
class AimState : public State<MainPlayerComponent> {
public:
	static AimState* Instance();

	virtual const char* GetName() const override { return "AimState"; }

	void Enter(MainPlayerComponent* owner) override;
	void Update(MainPlayerComponent* owner) override;
	void Exit(MainPlayerComponent* owner) override;
};
class ReRoadState : public State<MainPlayerComponent> {
public:
	static ReRoadState* Instance();

	virtual const char* GetName() const override { return "ReRoadState"; }

	void Enter(MainPlayerComponent* owner) override;
	void Update(MainPlayerComponent* owner) override;
	void Exit(MainPlayerComponent* owner) override;
};
class RhythmChangeState : public State<MainPlayerComponent> {
public:
	static RhythmChangeState* Instance();

	virtual const char* GetName() const override { return "RhythmChangeState"; }

	void Enter(MainPlayerComponent* owner) override;
	void Update(MainPlayerComponent* owner) override;
	void Exit(MainPlayerComponent* owner) override;
};

//damage--------------------------------------------------------
class HitState : public State<MainPlayerComponent> {
public:
	static HitState* Instance();
	virtual const char* GetName() const override { return "HitState"; }
	void Enter(MainPlayerComponent* owner) override;
	void Update(MainPlayerComponent* owner) override;
	void Exit(MainPlayerComponent* owner) override;
};
class StunState : public State<MainPlayerComponent> {
public:
	static StunState* Instance();
	virtual const char* GetName() const override { return "StunState"; }
	void Enter(MainPlayerComponent* owner) override;
	void Update(MainPlayerComponent* owner) override;
	void Exit(MainPlayerComponent* owner) override;
};
class DeadState : public State<MainPlayerComponent> {
public:
	static DeadState* Instance();
	virtual const char* GetName() const override { return "DeadState"; }
	void Enter(MainPlayerComponent* owner) override;
	void Update(MainPlayerComponent* owner) override;
	void Exit(MainPlayerComponent* owner) override;
};

//attack--------------------------------------------------------
class Attack1State : public State<MainPlayerComponent> {
public:
	static Attack1State* Instance();
	virtual const char* GetName() const override { return "Attack1State"; }
	void Enter(MainPlayerComponent* owner) override;
	void Update(MainPlayerComponent* owner) override;
	void Exit(MainPlayerComponent* owner) override;
};
class Attack2State : public State<MainPlayerComponent> {
public:
	static Attack2State* Instance();
	virtual const char* GetName() const override { return "Attack2State"; }
	void Enter(MainPlayerComponent* owner) override;
	void Update(MainPlayerComponent* owner) override;
	void Exit(MainPlayerComponent* owner) override;
};
class Skill1State : public State<MainPlayerComponent> {
public:
	static Skill1State* Instance();
	virtual const char* GetName() const override { return "Skill1State"; }
	void Enter(MainPlayerComponent* owner) override;
	void Update(MainPlayerComponent* owner) override;
	void Exit(MainPlayerComponent* owner) override;
};
class Skill2State : public State<MainPlayerComponent> {
public:
	static Skill2State* Instance();
	virtual const char* GetName() const override { return "Skill2State"; }
	void Enter(MainPlayerComponent* owner) override;
	void Update(MainPlayerComponent* owner) override;
	void Exit(MainPlayerComponent* owner) override;
};
class SpecialState : public State<MainPlayerComponent> {
public:
	static SpecialState* Instance();
	virtual const char* GetName() const override { return "SpecialState"; }
	void Enter(MainPlayerComponent* owner) override;
	void Update(MainPlayerComponent* owner) override;
	void Exit(MainPlayerComponent* owner) override;
};

