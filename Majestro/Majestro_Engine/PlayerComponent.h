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

enum : StateId { S_Idle = 0 , S_Walk =1 , S_Run = 2/*, S_Stun, S_Dead ...*/ };

using Flags = uint64_t;
enum : Flags {
	F_MOVE = 1ull << 0,
	F_STUN = 1ull << 1,
	F_DEAD = 1ull << 2,
	F_ATTACK = 1ull << 3,
	F_ANIM = 1ull << 4,
};

static std::unordered_map<std::string, uint64_t> gFlagByName = {
	{"F_MOVE", 1ull << 0} ,{"F_STUN", 1ull << 1}, {"F_DEAD", 1ull << 2},{"F_ATTACK", 1ull << 3}, { "F_ANIM", 1ull << 4 }
};

class MainPlayerComponent : public Component<MainPlayerComponent>
{
public:
	MainPlayerComponent():mFsm(this), mSpeed(0.0f), mFlags(0ull) {};
	MainPlayerComponent(const std::string& path): mFsm(this), mSpeed(0.0f), mFlags(0ull)  { 
		InitFSMFromJson(path);
	};

	StateMachine<MainPlayerComponent> mFsm{this};
	float mSpeed = 0.0f;
	uint64_t mFlags = 0ull;

	void InitFSMOnce();
	void InitFSMFromJson(const std::string& path);
};

class IdleState : public State<MainPlayerComponent> {
public:
	static IdleState* Instance() {                      
		static IdleState inst;
		return &inst;
	}
	void Enter(MainPlayerComponent* owner) override { std::cout << "Enter Idle\n"; }
	void Update(MainPlayerComponent* owner) override {
		
	}
	void Exit(MainPlayerComponent* owner) override { std::cout << "Exit Idle\n"; }
};

class WalkState : public State<MainPlayerComponent> {
public:
	static WalkState* Instance() {
		static WalkState inst;
		return &inst;
	}
	void Enter(MainPlayerComponent* owner) override { 
		owner->mFlags |= gFlagByName["F_MOVE"];
		std::cout << "Enter Walk\n"; 
	}
	void Update(MainPlayerComponent* owner) override {
		cout << "try--" << endl;
		owner->mFsm.ChangeState(owner,IdleState::Instance());
	}
	void Exit(MainPlayerComponent* owner) override { 
		std::cout << "Exit Walk\n"; 
	}
};

class RunState : public State<MainPlayerComponent> {
public:
	static RunState* Instance() {                      // [수정] Meyers' singleton (C++11+ 스레드 안전)
		static RunState inst;                          // 최초 호출 시 한 번만 생성
		return &inst;
	}
	void Enter(MainPlayerComponent* owner) override { std::cout << "Enter Run\n"; }
	void Update(MainPlayerComponent* owner) override {
		if (owner->mFsm.ChangeState(owner,IdleState::Instance())) return;
	}
	void Exit(MainPlayerComponent* owner) override { 
		std::cout << "Exit Run\n";
	}
};