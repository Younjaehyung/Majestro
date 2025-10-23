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
	{"F_MOVE", 1ull << 0} ,{"F_STUN", 1ull << 1}, {"F_DEAD", 1ull << 2},{"F_JUMP", 1ull << 3}, { "F_AIM", 1ull << 4 }
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

	void InitFSMOnce();
	void InitFSMFromJson(const std::string& path);

public:
	StateMachine<MainPlayerComponent> mFsm{this};
	float mSpeed = 0.0f;
	uint64_t mFlags = 0ull;
	float mStateTime=0.0f;
	float mDt=0.0f;

	float mJumpPower = 10.f;
	float mHight = 0.0f;
	float mGround = 0.0f;
};

class IdleState : public State<MainPlayerComponent> {
public:
	static IdleState* Instance() {                      
		static IdleState inst;
		return &inst;
	}
	void Enter(MainPlayerComponent* owner) override { 
		ClearFlag(owner->mFlags, gFlagByName["F_MOVE"]);
		std::cout << "Enter Idle\n"; 
	}
	void Update(MainPlayerComponent* owner) override {
		
	}
	void Exit(MainPlayerComponent* owner) override { 
		std::cout << "Exit Idle\n"; 
	}
};

class WalkState : public State<MainPlayerComponent> {
public:
	static WalkState* Instance() {
		static WalkState inst;
		return &inst;
	}
	void Enter(MainPlayerComponent* owner) override { 
		SetFlag(owner->mFlags, gFlagByName["F_MOVE"]);
		cout << owner->mFlags << endl;
		std::cout << "Enter Walk\n"; 
	}
	void Update(MainPlayerComponent* owner) override {
		//cout << "try--" << endl;
		owner->mFsm.ChangeState(owner,IdleState::Instance());
		//owner->mFsm.ChangeState(owner,RunState::Instance());
	}
	void Exit(MainPlayerComponent* owner) override { 
		//ClearFlag(owner->mFlags, gFlagByName["F_MOVE"]);
		std::cout << "Exit Walk\n"; 
	}
};

class RunState : public State<MainPlayerComponent> {
public:
	static RunState* Instance() {                      // [수정] Meyers' singleton (C++11+ 스레드 안전)
		static RunState inst;                          // 최초 호출 시 한 번만 생성
		return &inst;
	}
	void Enter(MainPlayerComponent* owner) override { 
		SetFlag(owner->mFlags, gFlagByName["F_MOVE"]);
		std::cout << "Enter Run\n"; 
	}
	void Update(MainPlayerComponent* owner) override {
		if (owner->mFsm.ChangeState(owner,WalkState::Instance())) return;
	}
	void Exit(MainPlayerComponent* owner) override { 
		std::cout << "Exit Run\n";
	}
};

class JumpState : public State<MainPlayerComponent> {
public:
	static JumpState* Instance() {                      // [수정] Meyers' singleton (C++11+ 스레드 안전)
		static JumpState inst;                          // 최초 호출 시 한 번만 생성
		return &inst;
	}
	void Enter(MainPlayerComponent* owner) override {
		owner->mStateTime = 0.0f;
		owner->mHight = 0.1f;
		SetFlag(owner->mFlags, gFlagByName["F_JUMP"]);
		std::cout << "Enter Jump\n";
	}
	void Update(MainPlayerComponent* owner) override {
		if (owner->mFsm.ChangeState(owner, IdleState::Instance())) return;
		float g = 9.8;
		owner->mHight += (owner->mJumpPower - g *owner->mStateTime)* owner->mDt ;
		cout << owner->mHight << endl;
	}
	void Exit(MainPlayerComponent* owner) override {
		owner->mHight = owner->mGround;
		
		std::cout << "Exit Jump\n";
	}
};